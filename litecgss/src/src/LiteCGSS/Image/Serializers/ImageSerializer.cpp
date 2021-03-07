#include <lodepng.h>
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

bool cgss::ImageFileSerializeMethod::load(sf::Image& image, const std::string& filename) {
	return image.loadFromFile(filename.c_str());
}

unsigned int cgss::ImageFileSerializeMethod::save(const sf::Image& image, const std::string& filename) {
	return lodepng_encode32_file(filename.c_str(), image.getPixelsPtr(), image.getSize().x, image.getSize().y);
}

bool cgss::ImageEmptySerializeMethod::load(sf::Image& image, unsigned int width, unsigned int height) {
	image.create(width, height, sf::Color(0, 0, 0, 0));
	return true;
}
