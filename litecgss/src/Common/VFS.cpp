#ifdef LITECGSS_USE_PHYSFS

#include "LiteCGSS/Common/VFS.h"
#include "LiteCGSS/Common/IOLock.h"


#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

#include <physfs.h>

namespace {
	long g_refCount = 0;
	std::string g_writeDir;

	void ensureInit() {
		if (PHYSFS_isInit()) {
			return;
		}
		if (PHYSFS_init(nullptr) == 0) {
			throw std::runtime_error{
				std::string{ "PhysFS init failed: " } +
				PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
			};
		}
		g_refCount = 0;
	}

	[[noreturn]] void throwPhysFSError(const std::string& context) {
		throw std::runtime_error{
			context + ": " + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
		};
	}

	std::string toString(std::string_view sv) {
		return std::string{ sv };
	}

	// ---- Streambuf backed by PHYSFS_File* ----
	class PhysfsStreambuf : public std::streambuf {
	public:
		explicit PhysfsStreambuf(PHYSFS_File* file) : m_file(file) {
			setg(m_buffer, m_buffer + kBufSize, m_buffer + kBufSize);
		}

		PhysfsStreambuf(const PhysfsStreambuf&) = delete;
		PhysfsStreambuf& operator=(const PhysfsStreambuf&) = delete;

		~PhysfsStreambuf() override {
			if (!m_file) return;
			cgss::vfs::runLocked([&] { PHYSFS_close(m_file); });
		}

	protected:
		int_type underflow() override {
			if (gptr() < egptr()) {
				return traits_type::to_int_type(*gptr());
			}
			return cgss::vfs::runLocked([&]() -> int_type {
				const auto bytesRead = PHYSFS_readBytes(m_file, m_buffer, kBufSize);
				if (bytesRead <= 0) {
					return traits_type::eof();
				}
				setg(m_buffer, m_buffer, m_buffer + bytesRead);
				return traits_type::to_int_type(*gptr());
			});
		}

		pos_type seekoff(off_type off, std::ios_base::seekdir dir,
		                 std::ios_base::openmode which) override {
			if (!(which & std::ios_base::in)) {
				return pos_type(off_type(-1));
			}
			return cgss::vfs::runLocked([&]() -> pos_type {
				PHYSFS_sint64 newPos = 0;
				const PHYSFS_sint64 cur = PHYSFS_tell(m_file)
				                          - static_cast<PHYSFS_sint64>(egptr() - gptr());
				switch (dir) {
					case std::ios_base::beg: newPos = off; break;
					case std::ios_base::cur: newPos = cur + off; break;
					case std::ios_base::end: newPos = PHYSFS_fileLength(m_file) + off; break;
					default: return pos_type(off_type(-1));
				}
				if (PHYSFS_seek(m_file, newPos) == 0) {
					return pos_type(off_type(-1));
				}
				setg(m_buffer, m_buffer + kBufSize, m_buffer + kBufSize);
				return pos_type(newPos);
			});
		}

		pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
			return seekoff(off_type(pos), std::ios_base::beg, which);
		}

	private:
		static constexpr std::size_t kBufSize = 4096;
		PHYSFS_File* m_file;
		char m_buffer[kBufSize];
	};

	class PhysfsIstream : public std::istream {
	public:
		explicit PhysfsIstream(PHYSFS_File* file) : std::istream(&m_buf), m_buf(file) {}
	private:
		PhysfsStreambuf m_buf;
	};

	// ---- Native fopen-backed ostream that writes to (writeDir + path) ----
	std::unique_ptr<std::ostream> openWriteNative(std::string_view path) {
		if (g_writeDir.empty()) {
			throw std::runtime_error{ "No write_dir set; cannot write '" + toString(path) + "'" };
		}
		const auto full = std::filesystem::path(g_writeDir) / std::filesystem::path(toString(path)).relative_path();
		std::filesystem::create_directories(full.parent_path());
		auto stream = std::make_unique<std::ofstream>(full, std::ios::binary | std::ios::trunc);
		if (!stream->is_open()) {
			throw std::runtime_error{ "Cannot open '" + full.string() + "' for writing" };
		}
		return stream;
	}

	// ---- Glob: brace expansion + fnmatch ----
	// Expand `{a,b}/x` -> ["a/x", "b/x"]. Handles nested braces.
	std::vector<std::string> braceExpand(const std::string& pattern) {
		const auto open = pattern.find('{');
		if (open == std::string::npos) {
			return { pattern };
		}
		// Find matching close brace.
		int depth = 1;
		std::size_t close = open + 1;
		for (; close < pattern.size() && depth > 0; ++close) {
			if (pattern[close] == '{') ++depth;
			else if (pattern[close] == '}') --depth;
		}
		if (depth != 0) {
			return { pattern };  // unbalanced — treat literally
		}
		--close;  // back up to the '}'
		const std::string prefix = pattern.substr(0, open);
		const std::string body = pattern.substr(open + 1, close - open - 1);
		const std::string suffix = pattern.substr(close + 1);
		// Split body on top-level commas.
		std::vector<std::string> alternatives;
		std::string current;
		int innerDepth = 0;
		for (const char c : body) {
			if (c == '{') ++innerDepth;
			else if (c == '}') --innerDepth;
			if (c == ',' && innerDepth == 0) {
				alternatives.push_back(current);
				current.clear();
			} else {
				current.push_back(c);
			}
		}
		alternatives.push_back(current);
		std::vector<std::string> out;
		for (const auto& alt : alternatives) {
			for (auto& expanded : braceExpand(prefix + alt + suffix)) {
				out.push_back(std::move(expanded));
			}
		}
		return out;
	}

	// Match a single path component against a single pattern component
	// supporting `*`, `?`, `[abc]`. Recursion is handled at a higher level.
	bool fnmatchSegment(const char* pat, const char* str) {
		while (*pat) {
			if (*pat == '*') {
				while (*pat == '*') ++pat;
				if (!*pat) return true;
				while (*str) {
					if (fnmatchSegment(pat, str)) return true;
					++str;
				}
				return false;
			}
			if (*pat == '?') {
				if (!*str) return false;
				++pat; ++str;
				continue;
			}
			if (*pat == '[') {
				const char* end = std::strchr(pat, ']');
				if (!end || !*str) return false;
				bool negate = false;
				const char* p = pat + 1;
				if (*p == '!' || *p == '^') { negate = true; ++p; }
				bool found = false;
				while (p < end) {
					if (p + 2 < end && p[1] == '-') {
						if (*str >= p[0] && *str <= p[2]) found = true;
						p += 3;
					} else {
						if (*str == *p) found = true;
						++p;
					}
				}
				if (found == negate) return false;
				pat = end + 1; ++str;
				continue;
			}
			if (*pat != *str) return false;
			++pat; ++str;
		}
		return *str == '\0';
	}

	// Split a path on '/' into components, dropping empties.
	std::vector<std::string> splitPath(std::string_view p) {
		std::vector<std::string> out;
		std::string cur;
		for (const char c : p) {
			if (c == '/') {
				if (!cur.empty()) { out.push_back(std::move(cur)); cur.clear(); }
			} else {
				cur.push_back(c);
			}
		}
		if (!cur.empty()) out.push_back(std::move(cur));
		return out;
	}

	void globRecurse(const std::string& base,
	                 const std::vector<std::string>& parts,
	                 std::size_t idx,
	                 std::vector<std::string>& out) {
		if (idx >= parts.size()) {
			if (!base.empty()) out.push_back(base);
			return;
		}
		const std::string& seg = parts[idx];
		const std::string baseForList = base.empty() ? std::string{ "/" } : base;
		const auto entries = cgss::vfs::enumerate(baseForList);

		if (seg == "**") {
			// Match zero or more directories. Try matching the rest right here…
			globRecurse(base, parts, idx + 1, out);
			// …and recurse into every subdirectory keeping ** in place.
			for (const auto& e : entries) {
				const auto child = base.empty() ? e : base + "/" + e;
				if (cgss::vfs::isDirectory(child)) {
					globRecurse(child, parts, idx, out);
				}
			}
			return;
		}

		for (const auto& e : entries) {
			if (!fnmatchSegment(seg.c_str(), e.c_str())) continue;
			const auto child = base.empty() ? e : base + "/" + e;
			if (idx + 1 == parts.size()) {
				out.push_back(child);
			} else if (cgss::vfs::isDirectory(child)) {
				globRecurse(child, parts, idx + 1, out);
			}
		}
	}
}  // namespace

namespace cgss::vfs {

	// Every public entry point is wrapped in runLocked so it serialises behind
	// the same lock as the embedder's other PhysFS callers (when an IO lock
	// runner has been installed). The lock is reentrant — leaf functions
	// calling each other (glob -> enumerate -> isDirectory -> stat) is fine.

	void mount(const std::string& source, const std::string& mountPoint, bool prepend) {
		runLocked([&] {
			ensureInit();
			const int appendFlag = prepend ? 0 : 1;
			if (PHYSFS_mount(source.c_str(), mountPoint.c_str(), appendFlag) == 0) {
				throwPhysFSError("PhysFS mount failed for '" + source + "' at '" + mountPoint + "'");
			}
			++g_refCount;
		});
	}

	void unmount(const std::string& source) {
		runLocked([&] {
			if (!PHYSFS_isInit()) return;
			if (PHYSFS_unmount(source.c_str()) == 0) {
				throwPhysFSError("PhysFS unmount failed for '" + source + "'");
			}
			if (--g_refCount <= 0) {
				g_refCount = 0;
			}
		});
	}

	void setWriteDir(const std::string& realDir) {
		runLocked([&] {
			ensureInit();
			// If we previously mounted a write_dir, unmount it before the swap.
			if (!g_writeDir.empty()) {
				PHYSFS_unmount(g_writeDir.c_str());
			}
			if (PHYSFS_setWriteDir(realDir.c_str()) == 0) {
				throwPhysFSError("PHYSFS_setWriteDir('" + realDir + "') failed");
			}
			// Mount as priority 0 so written files shadow the zip on subsequent reads.
			if (PHYSFS_mount(realDir.c_str(), "/", 0) == 0) {
				throwPhysFSError("PhysFS mount of write_dir '" + realDir + "' failed");
			}
			g_writeDir = realDir;
		});
	}

	const std::string& getWriteDir() {
		// Returned by reference for zero-copy use at the immediate call site.
		// The lock is acquired only for the duration of the snapshot read.
		static thread_local std::string snapshot;
		runLocked([&] { snapshot = g_writeDir; });
		return snapshot;
	}

	void shutdown() {
		runLocked([&] {
			if (!PHYSFS_isInit()) return;
			PHYSFS_deinit();
			g_refCount = 0;
			g_writeDir.clear();
		});
	}

	bool exists(std::string_view path) {
		return runLocked([&]() -> bool {
			return PHYSFS_isInit() && PHYSFS_exists(toString(path).c_str());
		});
	}

	std::optional<Stat> stat(std::string_view path) {
		return runLocked([&]() -> std::optional<Stat> {
			if (!PHYSFS_isInit()) return std::nullopt;
			PHYSFS_Stat raw{};
			if (PHYSFS_stat(toString(path).c_str(), &raw) == 0) return std::nullopt;
			Stat s{};
			s.filesize = raw.filesize;
			s.modtime = raw.modtime;
			s.createtime = raw.createtime;
			s.accesstime = raw.accesstime;
			s.readonly = raw.readonly;
			switch (raw.filetype) {
				case PHYSFS_FILETYPE_REGULAR:   s.filetype = FileType::Regular;   break;
				case PHYSFS_FILETYPE_DIRECTORY: s.filetype = FileType::Directory; break;
				case PHYSFS_FILETYPE_SYMLINK:   s.filetype = FileType::Symlink;   break;
				case PHYSFS_FILETYPE_OTHER:
				default:                        s.filetype = FileType::Other;     break;
			}
			return s;
		});
	}

	bool isDirectory(std::string_view path) {
		const auto s = stat(path);
		return s && s->filetype == FileType::Directory;
	}

	std::int64_t mtime(std::string_view path) {
		const auto s = stat(path);
		return s ? s->modtime : 0;
	}

	std::vector<std::string> enumerate(std::string_view directory) {
		return runLocked([&]() -> std::vector<std::string> {
			if (!PHYSFS_isInit()) return {};
			std::vector<std::string> result;
			char** list = PHYSFS_enumerateFiles(toString(directory).c_str());
			if (!list) return result;
			for (char** it = list; *it != nullptr; ++it) {
				result.emplace_back(*it);
			}
			PHYSFS_freeList(list);
			return result;
		});
	}

	std::vector<std::string> glob(std::string_view pattern) {
		std::vector<std::string> out;
		for (const auto& expanded : braceExpand(toString(pattern))) {
			const auto parts = splitPath(expanded);
			if (parts.empty()) continue;
			globRecurse("", parts, 0, out);
		}
		// Stable sort + dedup, matching what callers typically expect.
		std::sort(out.begin(), out.end());
		out.erase(std::unique(out.begin(), out.end()), out.end());
		return out;
	}

	Backend resolve(std::string_view path) {
		if (exists(path)) return Backend::Physfs;
		const auto p = std::filesystem::path(toString(path));
		std::error_code ec;
		if (std::filesystem::exists(p, ec)) return Backend::RealFs;
		return Backend::NotFound;
	}

	std::unique_ptr<std::istream> openRead(std::string_view path) {
		return runLocked([&]() -> std::unique_ptr<std::istream> {
			ensureInit();
			PHYSFS_File* file = PHYSFS_openRead(toString(path).c_str());
			if (!file) {
				throwPhysFSError("PhysFS openRead failed for '" + toString(path) + "'");
			}
			return std::make_unique<PhysfsIstream>(file);
		});
	}

	std::vector<char> loadFully(std::string_view path) {
		return runLocked([&]() -> std::vector<char> {
			ensureInit();
			PHYSFS_File* file = PHYSFS_openRead(toString(path).c_str());
			if (!file) {
				throwPhysFSError("PhysFS openRead failed for '" + toString(path) + "'");
			}
			const auto length = PHYSFS_fileLength(file);
			std::vector<char> buffer;
			if (length > 0) {
				buffer.resize(static_cast<std::size_t>(length));
				const auto got = PHYSFS_readBytes(file, buffer.data(), static_cast<PHYSFS_uint64>(length));
				if (got != length && !PHYSFS_eof(file)) {
					PHYSFS_close(file);
					throwPhysFSError("PhysFS readBytes failed for '" + toString(path) + "'");
				}
				buffer.resize(static_cast<std::size_t>(got));
			}
			PHYSFS_close(file);
			return buffer;
		});
	}

	std::unique_ptr<std::ostream> openWrite(std::string_view path) {
		// openWriteNative reads g_writeDir, which is mutated by setWriteDir
		// under the lock. Reading it bare would be a data race.
		return runLocked([&]() -> std::unique_ptr<std::ostream> {
			return openWriteNative(path);
		});
	}
}

#endif // LITECGSS_USE_PHYSFS
