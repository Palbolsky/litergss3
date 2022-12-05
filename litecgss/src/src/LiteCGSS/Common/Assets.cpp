#ifdef LITECGSS_USE_PHYSFS
#include <filesystem>
#include <iostream>
#include <string>
#include <stdexcept>
#include <optional>
#include <physfs.h>

#include "Assets.h"

long cgss::AssetsArchive::RefCounter = 0;

cgss::AssetFile& cgss::AssetFile::operator<<(const std::string& data) {
	if (m_mode != cgss::AssetFileMode::Write && m_mode != cgss::AssetFileMode::Append) {
		throw std::runtime_error( "Current file mode does not support writing" );
	}

	if (PHYSFS_writeBytes(m_file, data.c_str(), data.size()) != static_cast<PHYSFS_sint64>(data.size())) {
		throw std::runtime_error{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) };
	}

	return *this;
}

bool cgss::AssetFile::exists(const std::string& filename) {
	return PHYSFS_isInit() && PHYSFS_exists(filename.c_str());
}

std::optional<cgss::AssetFile::Stat> cgss::AssetFile::stat(const std::string& filename) {
	if (!PHYSFS_isInit()) {
		return {};
	}

	auto raw_result = PHYSFS_Stat {};
	const auto return_code = PHYSFS_stat(filename.c_str(), &raw_result);
	if (return_code == 0) {
		throw std::runtime_error{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) };
	}

	auto result = Stat {};
	result.filesize = raw_result.filesize;
	result.modtime = raw_result.modtime;
	result.createtime = raw_result.createtime;
	result.accesstime = raw_result.accesstime;
	result.readonly = raw_result.readonly;
	switch (raw_result.filetype) {
		case PHYSFS_FILETYPE_REGULAR: result.filetype = FileType::Regular; break;
		case PHYSFS_FILETYPE_DIRECTORY: result.filetype = FileType::Directory; break;
		case PHYSFS_FILETYPE_SYMLINK: result.filetype = FileType::Symlink; break;
		case PHYSFS_FILETYPE_OTHER:
		default: result.filetype = FileType::Other; break;
	}

	return result;
}

std::vector<std::string> cgss::AssetFile::enumerate(const std::string& directory) {
	if (!PHYSFS_isInit()) {
		return {};
	}

	auto result = std::vector<std::string>{};
	bool to_clear = false;
	char **list = PHYSFS_enumerateFiles(directory.c_str());
	char **filename_it;
	try {
		for (filename_it = list; *filename_it != NULL; filename_it++) {
			result.push_back(std::string { *filename_it });
		}
	} catch(...) {
		to_clear = true;
	}
	PHYSFS_freeList(list);
	if (to_clear) {
		result.clear();
	}

	return result;
}

cgss::AssetFile::AssetFile(const std::string& filename, const std::string& mode) :
	m_mode(computeModeOrThrow(mode)),
	m_file(openOrThrow(filename, m_mode)) {
}

cgss::AssetFile::AssetFile(AssetFile&& file) {
	*this = std::move(file);
}

cgss::AssetFile& cgss::AssetFile::operator=(AssetFile&& file) {
	cleanup();
	m_file = file.m_file;
	file.m_file = nullptr;
	file.m_mode = AssetFileMode::Closed;
	return *this;
}

void cgss::AssetFile::close() {
	cleanup();
}

std::vector<char> cgss::AssetFile::load(uint32_t size) {
	if (m_mode != AssetFileMode::Read) {
		throw std::runtime_error( "Current file mode does not support reading" );
	}

	auto buffer = std::vector<char> {};
	buffer.resize(size);
	if (PHYSFS_readBytes(m_file, &buffer[0], size) != size && !PHYSFS_eof(m_file)) {
		throw std::runtime_error{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) };
	}
	return buffer;
}

std::vector<char> cgss::AssetFile::fullLoad() {
	return load(PHYSFS_fileLength(m_file));
}

void cgss::AssetFile::seek(uint64_t position) {
	PHYSFS_seek(m_file, position);
}

int64_t cgss::AssetFile::tell() const {
	return PHYSFS_tell(m_file);
}

cgss::AssetFile::~AssetFile() {
	cleanup();
}

void cgss::AssetFile::cleanup() {
	PHYSFS_close(m_file);
	m_file = nullptr;
	m_mode = AssetFileMode::Closed;
}

cgss::AssetFileMode cgss::AssetFile::computeModeOrThrow(const std::string& mode) {
	if (mode == "r" || mode == "rb") {
		return AssetFileMode::Read;
	} else if (mode == "w" || mode == "wb") {
		return AssetFileMode::Write;
	} else if (mode == "a" || mode == "ab") {
		return AssetFileMode::Append;
	}
	throw std::runtime_error{ std::string{ "Unsupported mode '" + mode + "'"}.c_str() };
}

PHYSFS_file* cgss::AssetFile::openOrThrow(const std::string& filename, AssetFileMode mode) {
	if (!PHYSFS_isInit()) {
		AssetsArchive::init();
	}

	if (!exists(filename)) {
		throw std::runtime_error{ "File \"" + filename + "\" does not exist in current PhysFS context"};
	}

	PHYSFS_file* file = nullptr;

	switch (mode) {
	case AssetFileMode::Read:
		file = PHYSFS_openRead(filename.c_str());
		break;
	case AssetFileMode::Write: {
		file = PHYSFS_openWrite(filename.c_str());

		const auto& path = std::filesystem::path(filename).parent_path();
		if (!path.empty() && path != "/") {
			if (PHYSFS_mkdir(path.c_str()) == 0) {
				throw std::runtime_error{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) };
			}
		}
	}
		break;
	case AssetFileMode::Append:
		file = PHYSFS_openAppend(filename.c_str());
		break;
	default:
		throw std::runtime_error{ "Unknown file mode" };
	}

	if (file == nullptr) {
		throw std::runtime_error{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) };
	}
	return file;
}

cgss::AssetsArchive::AssetsArchive(std::string mountDirectory) :
	m_mountedDirectory(mountOrThrow(std::move(mountDirectory))) {
}

int cgss::AssetsArchive::init() {
	if (PHYSFS_isInit()) {
		return 1;
	}
	const int result = PHYSFS_init(nullptr);
	if (result == 0) {
		throw std::runtime_error("UNABLE TO INIT PHYSFS");
	}
	return result ? 1 : 0;
}

int cgss::AssetsArchive::deinit() {
	if (--RefCounter < 0 && PHYSFS_isInit()) {
		RefCounter = 0;
		return PHYSFS_deinit();
	}
	return 0;
}

cgss::AssetsArchive::~AssetsArchive() {
	if (!m_mountedDirectory.empty()) {
		cleanup();
		deinit();
	}
}

std::string cgss::AssetsArchive::mountOrThrow(std::string mountDirectory) {
	if (RefCounter == 0) {
		if (init() == 1) {
			RefCounter++;
		}
	}

	if (!PHYSFS_mount(mountDirectory.c_str(), "/", 1)) {
		throw std::runtime_error{ "PhysFS mount error at '/' using zip file '" + mountDirectory + "' : " + std::string{ PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) } };
	}
	return mountDirectory;
}

void cgss::AssetsArchive::cleanup() {
	if (!m_mountedDirectory.empty()) {
		if (PHYSFS_isInit()) {
			PHYSFS_unmount(m_mountedDirectory.c_str());
		}
		m_mountedDirectory = "";
	}
}

int cgss::AssetWriter::SetDirectory(const std::string& directory) {
	if (!PHYSFS_isInit()) {
		throw std::runtime_error("PhysFS is not initialized");
	}
	return PHYSFS_setWriteDir(directory.c_str());
}

#endif
