#include <iostream>
#include <lodepng.h>
#include "LiteCGSS/Common/Assets.h"
#include "ImageSerializer.h"

bool cgss::ImageMemorySerializeMethod::load(sf::Image& image, const MemorySerializerData& rawData) {
	return image.loadFromMemory(rawData.first, rawData.second);
}

cgss::MemorySerializerData cgss::ImageMemorySerializeMethod::save(const sf::Image& image) {
	unsigned char* out = nullptr;
	size_t size;
	if (lodepng_encode32(&out, &size, image.getPixelsPtr(), image.getSize().x, image.getSize().y) != 0) {
		free(out);
		return { nullptr, 0u };
	}
	return { out, size };
}

bool cgss::ImageEmptySerializeMethod::load(sf::Image& image, unsigned int width, unsigned int height) {
	image.create(width, height, sf::Color(0, 0, 0, 0));
	return true;
}

#ifdef LITECGSS_USE_PHYSFS
bool cgss::ImageAssetSerializeMethod::load(sf::Image& image, const std::string& filename) {
	if (!AssetFile::exists(filename)) {
		return image.loadFromFile(filename.c_str());
	}
	try {
		const auto buffer = AssetFile { filename, "r" }.fullLoad();
		return image.loadFromMemory(&buffer[0], buffer.size());
	} catch (const std::exception& err) {
		std::cerr << "Error while loading asset \"" << filename << "\" : " << err.what() << std::endl;
		return false;
	}
}

unsigned int cgss::ImageAssetSerializeMethod::save(const sf::Image& image, const std::string& filename) {
	// Currently unsupported in an archive
	// Should we ?
	return lodepng_encode32_file(filename.c_str(), image.getPixelsPtr(), image.getSize().x, image.getSize().y);
}
#else
bool cgss::ImageFilesystemSerializeMethod::load(sf::Image& image, const std::string& filename) {
	return image.loadFromFile(filename.c_str());
}

unsigned int cgss::ImageFilesystemSerializeMethod::save(const sf::Image& image, const std::string& filename) {
	return lodepng_encode32_file(filename.c_str(), image.getPixelsPtr(), image.getSize().x, image.getSize().y);
}
#endif
