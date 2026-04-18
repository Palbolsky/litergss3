#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Color.hpp>
#include "Texture.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Common/Internal/SfRectConv.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "LiteCGSS/Image/Image.h"

cgss::Texture::Texture() :
	m_texture(std::make_shared<sf::Texture>()),
	m_image(std::make_unique<sf::Image>()) {
}

cgss::Texture::Texture(const Texture& other) :
	m_loaded(other.m_loaded),
	m_texture(other.m_texture),
	m_image(std::make_unique<sf::Image>(*other.m_image)) {
}

cgss::Texture::Texture(Texture&& other) noexcept = default;

cgss::Texture& cgss::Texture::operator=(const Texture& other) {
	m_loaded = other.m_loaded;
	m_texture = other.m_texture;
	*m_image = *other.m_image;
	return *this;
}

cgss::Texture& cgss::Texture::operator=(Texture&& other) noexcept = default;

cgss::Texture::~Texture() = default;

cgss::Texture cgss::Texture::create(const DisplayWindow& window, const std::string& filename) {
	auto result = Texture{};
	result.m_texture->setSmooth(window.smoothScreen());
	TextureFileSerializeMethod::load(*result.m_texture, filename, *result.m_image);
	return result;
}

cgss::Texture cgss::Texture::create(const std::string& filename) {
	auto result = Texture{};
	TextureFileSerializeMethod::load(*result.m_texture, filename, *result.m_image);
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
	m_loaded = loader.load(*m_texture, *m_image);
	return m_loaded;
}

cgss::Texture cgss::Texture::clone() const {
	auto result = Texture{};
	result.m_texture = m_texture;
	return result;
}

void cgss::Texture::update(const sf::Image* image) {
	if (image != nullptr) {
		//Explicit copy
		*m_image = *image;
	}
	m_texture->update(*m_image);
}

void cgss::Texture::update(const Image& image) {
	update(&image.m_image);
}

void cgss::Texture::blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect) {
	if(hasImage()) {
		m_image->copy(*srcTexture.m_image, x, y, cgss::internal::toSf(rect.getValue()));
	}
}

void cgss::Texture::fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color) {
	const long xStart = x < 0 ? 0 : x;
	const long xEnd = x + width;

	const long yStart = y < 0 ? 0 : y;
	const long yEnd = y + height;

	for (long yIt = yStart; yIt < yEnd; yIt++) {
		for (long xIt = xStart; xIt < xEnd; xIt++) {
			m_image->setPixel(xIt, yIt, color);
		}
	}
	update();
}

void cgss::Texture::clearRect(long x, long y, unsigned int width, unsigned int height) {
	static const auto BLACK_COLOR = sf::Color{0, 0, 0, 0};
	fillRect(x, y, width, height, BLACK_COLOR);
}

unsigned int cgss::Texture::write(TextureSaver& saver) const {
	return saver.save(*m_texture);
}

unsigned int cgss::Texture::getNpotSize(unsigned int size) {
	unsigned int powerOfTwo = 1;
	while (powerOfTwo < size) {
		powerOfTwo *= 2;
	}
	return powerOfTwo;
}

cgss::Texture::Size cgss::Texture::getSize() const {
	const auto sz = m_texture->getSize();
	return { sz.x, sz.y };
}

bool cgss::Texture::create(int sw, int sh) {
	return m_texture->create(sw, sh);
}

std::weak_ptr<sf::Texture> cgss::Texture::weak() { return m_texture; }
std::shared_ptr<sf::Texture> cgss::Texture::ref() { return m_texture; }
sf::Texture& cgss::Texture::raw() { return *m_texture.get(); }
const sf::Texture& cgss::Texture::raw() const { return *m_texture.get(); }

cgss::TextureFactorNpot cgss::Texture::factorNpot() const {
	return {
		static_cast<double>(m_texture->getSize().x) / getNpotSize(m_texture->getSize().x),
		static_cast<double>(m_texture->getSize().y) / getNpotSize(m_texture->getSize().y)
	};
}

bool cgss::Texture::hasImage() const {
	return m_image->getSize().x != 0;
}
