#ifndef CGSS_VFS_H
#define CGSS_VFS_H

#ifdef LITECGSS_USE_PHYSFS

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cgss::vfs {
	enum class Backend {
		Physfs,
		RealFs,
		NotFound
	};

	enum class FileType {
		Regular,
		Directory,
		Symlink,
		Other
	};

	struct Stat {
		std::int64_t filesize;
		std::int64_t modtime;
		std::int64_t createtime;
		std::int64_t accesstime;
		FileType filetype;
		bool readonly;
	};

	// ---- Mount lifecycle ----
	// Mount a zip archive or directory. `prepend=true` makes this take priority
	// over earlier mounts at the same point. Auto-inits PhysFS on first call.
	void mount(const std::string& source, const std::string& mountPoint = "/", bool prepend = false);
	void unmount(const std::string& source);

	// Set the writable native directory. Does TWO things atomically:
	//   1. PHYSFS_setWriteDir  — for any code using PhysFS write API.
	//   2. PHYSFS_mount(dir, "/", prepend=true) — so written files become
	//      visible to subsequent reads (write_dir shadows other mounts).
	// Writes themselves should go through openWrite() (native fopen) so that
	// rename/fsync/atomic-temp-rename patterns work — PhysFS write API can't.
	void setWriteDir(const std::string& realDir);
	const std::string& getWriteDir();

	// Refcounted shutdown. Safe to call multiple times.
	void shutdown();

	// ---- Queries ----
	bool exists(std::string_view path);
	bool isDirectory(std::string_view path);
	std::optional<Stat> stat(std::string_view path);
	std::int64_t mtime(std::string_view path);  // 0 if missing
	std::vector<std::string> enumerate(std::string_view directory);

	// Shell-style glob with `*`, `**`, `?`, `[abc]`, `{a,b}`.
	// Returns paths relative to PhysFS root.
	std::vector<std::string> glob(std::string_view pattern);

	// One-shot resolution. Used by the Ruby shim so each File/Dir override
	// can branch ONCE on the backend instead of try/rescue chains.
	Backend resolve(std::string_view path);

	// ---- IO ----
	// Read via PhysFS — searches all mounts in priority order, so the
	// write_dir mount automatically shadows the zip's stock copy.
	std::unique_ptr<std::istream> openRead(std::string_view path);

	// Convenience: read the whole file into a buffer. Used by callers that
	// need the bytes to outlive the read (e.g. fonts retain the buffer), and
	// by the asset loaders, which decode from memory on every backend.
	std::vector<char> loadFully(std::string_view path);

	// Write via NATIVE fopen on (writeDir + path), preserving rename/fsync.
	// Throws std::runtime_error if no write_dir is set.
	std::unique_ptr<std::ostream> openWrite(std::string_view path);
}

#endif // LITECGSS_USE_PHYSFS
#endif
