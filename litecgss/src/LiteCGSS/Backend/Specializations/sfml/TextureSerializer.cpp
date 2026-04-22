// SFML implementations of the Texture serializer family — `template<>`
// specialisations of the basic_TextureXxxSerializeMethod<SFMLBackend>::*
// methods. Selected by CMake when CGSS_BACKEND=sfml (folder-based per-backend
// specialization). The raylib stub equivalents live in
// Backend/Specializations/raylib/TextureSerializer.cpp.

#include <iostream>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Color.hpp>
#include <lodepng.h>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"

static unsigned int TextureLodePngDecode(sf::Texture& texture, const char* rawDataOrFilename, long from_memory_size) {
	unsigned char* out = nullptr;
	unsigned w = 0;
	unsigned h = 0;
	unsigned int result = 0;

	if (from_memory_size > 0) {
		result = lodepng_decode32(&out, &w, &h, reinterpret_cast<const unsigned char*>(rawDataOrFilename), from_memory_size);
	} else {
		result = lodepng_decode32_file(&out, &w, &h, rawDataOrFilename);
	}

	if (!result) {
		try {
			texture.create(w, h);
			texture.update(reinterpret_cast<sf::Uint8*>(out), w, h, 0, 0);
		} catch (...) {
			result = static_cast<unsigned int>(-1);
		}
	}
	free(out);
	return result;
}

template <>
bool cgss::basic_TextureMemorySerializeMethod<cgss::backend::SFMLBackend>::load(sf::Texture& texture, const MemorySerializerData& rawData, sf::Image&) {
	const auto errorCode = TextureLodePngDecode(texture, reinterpret_cast<const char*>(rawData.first), rawData.second);
	if (errorCode == 0) {
		return true;
	}
	return texture.loadFromMemory(rawData.first, rawData.second);
}

template <>
cgss::MemorySerializerData cgss::basic_TextureMemorySerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Texture& texture) {
	sf::Image img = texture.copyToImage();

	unsigned char* out = nullptr;
	std::size_t size;
	const unsigned int errorCode = lodepng_encode32(&out, &size, img.getPixelsPtr(), img.getSize().x, img.getSize().y) != 0;
	if (errorCode != 0) {
		free(out);
		std::cerr << "LodePNG error : " <<  lodepng_error_text(errorCode) << std::endl;
		return { nullptr, 0u };
	}
	return { out, size };
}

template <>
bool cgss::basic_TextureEmptySerializeMethod<cgss::backend::SFMLBackend>::load(sf::Texture& texture, unsigned int width, unsigned int height, sf::Image& image) {
	image.create(width, height, sf::Color(0, 0, 0, 0));
	return texture.loadFromImage(image);
}

#ifdef LITECGSS_USE_PHYSFS
template <>
bool cgss::basic_TextureAssetSerializeMethod<cgss::backend::SFMLBackend>::load(sf::Texture& texture, const std::string& filename, sf::Image&) {
	if (!AssetFile::exists(filename)) {
		return texture.loadFromFile(filename.c_str());
	}
	try {
		const auto buffer = AssetFile { filename, "r" }.fullLoad();
		return texture.loadFromMemory(&buffer[0], buffer.size());
	} catch (const std::exception& err) {
		std::cerr << "Error while loading asset \"" << filename << "\" : " << err.what() << std::endl;
		return false;
	}
}

template <>
unsigned int cgss::basic_TextureAssetSerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Texture& texture, const std::string& filename) {
	// Currently unsupported in an archive
	// Should we ?
	sf::Image img = texture.copyToImage();
	return lodepng_encode32_file(filename.c_str(), img.getPixelsPtr(), img.getSize().x, img.getSize().y);
}
#else
template <>
bool cgss::basic_TextureFilesystemSerializeMethod<cgss::backend::SFMLBackend>::load(sf::Texture& texture, const std::string& filename, sf::Image&) {
	const auto errorCode = TextureLodePngDecode(texture, filename.c_str(), 0);
	if (errorCode == 0) {
		return true;
	}
	return texture.loadFromFile(filename.c_str());
}

template <>
unsigned int cgss::basic_TextureFilesystemSerializeMethod<cgss::backend::SFMLBackend>::save(const sf::Texture& texture, const std::string& filename) {
	sf::Image img = texture.copyToImage();
	return lodepng_encode32_file(filename.c_str(), img.getPixelsPtr(), img.getSize().x, img.getSize().y);
}
#endif
