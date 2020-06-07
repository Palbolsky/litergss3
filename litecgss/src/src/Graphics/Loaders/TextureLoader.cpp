#include <iostream>
#include "lodepng.h"
#include "TextureLoader.h"

unsigned int cgss::TextureLoader::lodePngDecode(sf::Texture& texture, const char* rawDataOrFilename, long from_memory_size) {
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
		texture.create(w, h);
		texture.update(reinterpret_cast<sf::Uint8*>(out), w, h, 0, 0);
	}
	free(out);
	return result;
}


cgss::TextureFileLoader::TextureFileLoader(std::string filename) :
	m_filename(std::move(filename)) {
}

cgss::TextureEmptyLoader::TextureEmptyLoader(unsigned int width, unsigned int height) :
	m_width(width),
	m_height(height) {
}

cgss::TextureMemoryLoader::TextureMemoryLoader(std::vector<unsigned char> rawData) :
	m_memory(std::move(rawData)) {
}

bool cgss::TextureMemoryLoader::load(sf::Texture& texture, sf::Image& image) const {
	return instantLoad(m_memory, texture, image);
}

bool cgss::TextureMemoryLoader::instantLoad(const std::vector<unsigned char>& rawData, sf::Texture& texture, sf::Image& image) {
	const bool errorCode = lodePngDecode(texture, reinterpret_cast<const char*>(&rawData[0]), rawData.size());
	if (errorCode == 0) {
		return texture.loadFromMemory(&rawData[0], rawData.size());
	}
	std::cerr << "LodePNG error : " << lodepng_error_text(errorCode) << std::endl;
	return false;
}

bool cgss::TextureFileLoader::load(sf::Texture& texture, sf::Image& image) const {
	return instantLoad(m_filename, texture, image);
}

void cgss::TextureFileLoader::save(const sf::Texture& texture) {
	m_writtenStatus = instantSave(m_filename, texture);
}

std::vector<unsigned char> cgss::TextureMemoryLoader::instantSave(const sf::Texture& texture) {
	std::vector<unsigned char> memory;
	sf::Image img = texture.copyToImage();

	unsigned char* out = nullptr;
	std::size_t size;
	const unsigned int errorCode = lodepng_encode32(&out, &size, img.getPixelsPtr(), img.getSize().x, img.getSize().y) != 0;

	if (errorCode == 0) {
		//Involves a memory allocation, so could throw, meaning that we have to handle exceptions if we want to correctly free "out"
		try {
			memory = std::vector<unsigned char>{ out, out + size };
		} catch (std::exception&) {
			//Nothing special here (it could be logging maybe ?)
		}
	} else {
		std::cerr << "LodePNG error : " <<  lodepng_error_text(errorCode) << std::endl;
	}
	
	free(out);
	return memory;
}

void cgss::TextureMemoryLoader::save(const sf::Texture& texture) {
	m_memory = std::move(instantSave(texture));
}

bool cgss::TextureFileLoader::instantLoad(const std::string& filename, sf::Texture& texture, sf::Image& image) {
	const auto errorCode = lodePngDecode(texture, filename.c_str(), 0);
	if (errorCode == 0) {
		return texture.loadFromFile(filename.c_str());
	}
	std::cerr << "LodePNG error : " <<  lodepng_error_text(errorCode) << std::endl;
	return true;
}

bool cgss::TextureEmptyLoader::load(sf::Texture& texture, sf::Image& image) const {
	image.create(m_width, m_height, sf::Color(0, 0, 0, 0));
	return texture.loadFromImage(image);
}

unsigned int cgss::TextureFileLoader::instantSave(const std::string& filename, const sf::Texture& texture) {
	sf::Image img = texture.copyToImage();
	return lodepng_encode32_file(filename.c_str(), img.getPixelsPtr(), img.getSize().x, img.getSize().y);
}
