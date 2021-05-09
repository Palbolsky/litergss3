#ifndef CGSS_ASSETS_H
#define CGSS_ASSETS_H

#ifdef LITECGSS_USE_PHYSFS
#include <string>
#include <vector>

struct PHYSFS_file;

namespace cgss {
	enum class AssetFileMode {
		Closed,
		Read,
		Write,
		Append
	};

	class AssetFile {
	public:
		AssetFile(const std::string& filename, const std::string& mode);
		AssetFile(const AssetFile&) = delete;
		AssetFile(AssetFile&& file);

		AssetFile& operator=(const AssetFile&) = delete;
		AssetFile& operator=(AssetFile&& file);

		static bool exists(const std::string& filename);

		std::vector<char> load(uint32_t size);
		std::vector<char> fullLoad();

		AssetFile& operator<<(const std::string& data);

		void seek(uint64_t position);
		int64_t tell() const;

		void close();

		virtual ~AssetFile();
	private:
		AssetFileMode m_mode = AssetFileMode::Closed;
		PHYSFS_file* m_file = nullptr;

		void cleanup();
		static PHYSFS_file* openOrThrow(const std::string& filename, AssetFileMode mode);
		static AssetFileMode computeModeOrThrow(const std::string& mode);
	};

	class AssetsArchive {
	public:
		AssetsArchive(std::string mountDirectory);
		AssetsArchive(const AssetsArchive&) = delete;
		AssetsArchive(AssetsArchive&& file) = delete;

		AssetsArchive& operator=(const AssetsArchive&) = delete;
		AssetsArchive& operator=(AssetsArchive&& archive) = delete;

		virtual ~AssetsArchive();
		static int init();
		static int deinit();
	private:
		static long RefCounter;
		std::string m_mountedDirectory;

		static std::string mountOrThrow(std::string mountDirectory);
		void cleanup();
	};

	namespace AssetWriter {
		int SetDirectory(const std::string& directory);
	}
}
#endif

#endif