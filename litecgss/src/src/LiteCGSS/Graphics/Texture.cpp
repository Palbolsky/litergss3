#include "Texture.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Graphics/Loaders/TextureLoader.h"

cgss::Texture::Texture() :
	m_texture(std::make_shared<sf::Texture>()) {
}

cgss::Texture cgss::Texture::create(const DisplayWindow& window, const std::string& filename) {
	auto result = Texture{};
	result.m_texture->setSmooth(window.smoothScreen());
	TextureFileLoader::instantLoad(filename, *result.m_texture, result.m_image);
	return result;
}

cgss::Texture cgss::Texture::create(const std::string& filename) {
	auto result = Texture{};
	TextureFileLoader::instantLoad(filename, *result.m_texture, result.m_image);
	return result;
}

cgss::Texture cgss::Texture::create(sf::Texture texture) {
	auto result = Texture {};
	result.m_texture = std::make_shared<sf::Texture>(std::move(texture));
	result.m_loaded = true;
	return result;
}

void cgss::Texture::setSmooth(bool smooth) {
	m_texture->setSmooth(smooth);
}

bool cgss::Texture::load(const TextureLoader& loader) {
	m_loaded = loader.load(*m_texture, m_image);
	return m_loaded;
}

cgss::Texture cgss::Texture::clone() const {
	auto result = Texture{};
	result.m_texture = m_texture;
	return result;
}

void cgss::Texture::update(sf::Image* image) {
	if (image != nullptr) {
		//Explicit copy
		m_image = *image;
	}
	m_texture->update(m_image);
}

void cgss::Texture::blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect) {
	if(hasImage()) {
		m_image.copy(srcTexture.m_image, x, y, rect.getValue());
	}
}

void cgss::Texture::fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color) {
	const long xStart = x < 0 ? 0 : x;
	const long xEnd = x + width;

	const long yStart = y < 0 ? 0 : y;
	const long yEnd = y + height;

	for (long yIt = yStart; yIt < yEnd; yIt++) {
		for (long xIt = xStart; xIt < xEnd; xIt++) {
			m_image.setPixel(xIt, yIt, color);
		}
	}
	update();
}

void cgss::Texture::clearRect(long x, long y, unsigned int width, unsigned int height) {
	static const auto BLACK_COLOR = sf::Color{0, 0, 0, 0};
	fillRect(x, y, width, height, BLACK_COLOR);
}

void cgss::Texture::write(TextureSaver& saver) const {
	saver.save(*m_texture);
}

std::vector<unsigned char> cgss::Texture::toRawDataPng() const {
	return TextureMemoryLoader::instantSave(*m_texture);
}

unsigned int cgss::Texture::toFile(const std::string& filename) const {
	return TextureFileLoader::instantSave(filename, *m_texture);
}
