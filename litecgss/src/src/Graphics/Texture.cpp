#include "lodepng.h"
#include "Texture.h"
#include "Common/Rectangle.h"

bool cgss::Texture::loadPng(sf::Texture& text, const char* rawDataOrFilename, long from_memory_size) {
	unsigned char* out = nullptr;
	unsigned w = 0;
	unsigned h = 0;
	bool result = true;

	if (from_memory_size > 0) {
		result = lodepng_decode32(&out, &w, &h, reinterpret_cast<const unsigned char*>(rawDataOrFilename), from_memory_size) == 0;
	} else {
		result = lodepng_decode32_file(&out, &w, &h, rawDataOrFilename) == 0;
	}

	if (result) {
		text.create(w, h);
		text.update(reinterpret_cast<sf::Uint8*>(out), w, h, 0, 0);
	}
	free(out);
	return result;
}

bool cgss::Texture::load(const std::string& data, unsigned int width, unsigned int height, TextureLoadMethod loadMethod) {
	//TODO externalize in several texture loaders + polymorphism
	bool result = false;

	switch(loadMethod) {
	case TextureLoadMethod::File:
		if(!loadPng(m_texture, data.c_str(), 0)) {
			result = m_texture.loadFromFile(data.c_str());
		} else {
			result = true;
		}
		break;

	case TextureLoadMethod::Memory:
		if(!loadPng(m_texture, data.c_str(), data.size())) {
			result = m_texture.loadFromMemory(data.c_str(), data.size());
		} else {
			result = true;
		}
		break;

	case TextureLoadMethod::New:
		m_image.create(width, height, sf::Color(0, 0, 0, 0));
		result = m_texture.loadFromImage(m_image);
		break;

	default:
		break;
	}

	m_loaded = result;
	//TODO texture.setSmooth from global config ?
	return m_loaded;
}

void cgss::Texture::update() {
	if(hasImage()) {
		m_texture.update(m_image);
	}
}

void cgss::Texture::blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect) {
	if(hasImage()) {
		m_image.copy(srcTexture.m_image, x, y, rect.getRect());
	}
}

void cgss::Texture::fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color) {
	const long xStart = x < 0 ? 0 : x;
	const long xEnd = x + width;

	const long yStart = y < 0 ? 0 : y;
	const long yEnd = y + height;

	for(long yIt = yStart; yIt < yEnd; yIt++) {
		for (long xIt = xStart; xIt < xEnd; xIt++) {
			m_image.setPixel(xIt, yIt, color);
		}
	}
}

void cgss::Texture::clearRect(long x, long y, unsigned int width, unsigned int height) {
	static const auto BLACK_COLOR = sf::Color{0, 0, 0, 0};
	fillRect(x, y, width, height, BLACK_COLOR);
}


std::vector<unsigned char> cgss::Texture::toRawDataPng() {
	sf::Image img = m_texture.copyToImage();

	unsigned char* out = nullptr;
	std::size_t size;
	const auto success = lodepng_encode32(&out, &size, img.getPixelsPtr(), img.getSize().x, img.getSize().y) != 0;

	auto result = std::vector<unsigned char>{};
	if(success) {
		//Involves a memory allocation, so could throw, meaning that we have to handle exceptions if we want to correctly free "out"
		try {
			result = std::vector<unsigned char>(out, out + size);
		} catch (std::exception&) {
			//Nothing special here (it could be logging maybe ?)
		}
	}
	free(out);
	return result;
}

int cgss::Texture::toFile(const std::string& filename) {
	sf::Image img = m_texture.copyToImage();
	return lodepng_encode32_file(filename.c_str(), img.getPixelsPtr(), img.getSize().x, img.getSize().y);
}

void cgss::Texture::setShaderUniform(sf::Shader& shader, const std::string& attribute) const {
	shader.setUniform(attribute, m_texture);
}

std::unique_ptr<sf::Sprite> cgss::Texture::createSprite() {
	return std::make_unique<sf::Sprite>(m_texture);
}
