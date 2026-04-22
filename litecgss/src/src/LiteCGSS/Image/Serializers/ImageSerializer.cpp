// TODO(raylib, TEMPORARY): Image serializer impls are SFML-typed
// (sf::Image::loadFromFile/loadFromMemory/create). To remove the guard,
// add per-method specializations for raylib using LoadImage /
// LoadImageFromMemory / GenImageColor; encode-to-PNG via lodepng is already
// backend-agnostic and reusable.
#ifdef CGSS_BACKEND_SFML

#include <iostream>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Color.hpp>
#include <lodepng.h>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "ImageSerializer.h"

template <>
bool cgss::basic_ImageMemorySerializeMethod<cgss::backend::SFMLBackend>::load(sf::Image& image, const MemorySerializerData& rawData) {
	return image.loadFromMemory(rawData.first, rawData.second);
}

template <>
cgss::MemorySerializerData cgss::basic_ImageMemorySerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Image& image) {
	unsigned char* out = nullptr;
	size_t size;
	if (lodepng_encode32(&out, &size, image.getPixelsPtr(), image.getSize().x, image.getSize().y) != 0) {
		free(out);
		return { nullptr, 0u };
	}
	return { out, size };
}

template <>
bool cgss::basic_ImageEmptySerializeMethod<cgss::backend::SFMLBackend>::load(sf::Image& image, unsigned int width, unsigned int height) {
	image.create(width, height, sf::Color(0, 0, 0, 0));
	return true;
}

#ifdef LITECGSS_USE_PHYSFS
template <>
bool cgss::basic_ImageAssetSerializeMethod<cgss::backend::SFMLBackend>::load(sf::Image& image, const std::string& filename) {
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

template <>
unsigned int cgss::basic_ImageAssetSerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Image& image, const std::string& filename) {
	// Currently unsupported in an archive
	// Should we ?
	return lodepng_encode32_file(filename.c_str(), image.getPixelsPtr(), image.getSize().x, image.getSize().y);
}
#else
template <>
bool cgss::basic_ImageFilesystemSerializeMethod<cgss::backend::SFMLBackend>::load(sf::Image& image, const std::string& filename) {
	return image.loadFromFile(filename.c_str());
}

template <>
unsigned int cgss::basic_ImageFilesystemSerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Image& image, const std::string& filename) {
	return lodepng_encode32_file(filename.c_str(), image.getPixelsPtr(), image.getSize().x, image.getSize().y);
}
#endif

#endif  // CGSS_BACKEND_SFML
