#include "Font.h"
#include <SFML/Graphics/Font.hpp>

cgss::Font::Font() : m_font(std::make_unique<sf::Font>()) {}

cgss::Font::Font(const Font& other) : m_font(std::make_unique<sf::Font>(*other.m_font)) {}

cgss::Font::Font(Font&& other) noexcept = default;

cgss::Font& cgss::Font::operator=(const Font& other) {
	*m_font = *other.m_font;
	return *this;
}

cgss::Font& cgss::Font::operator=(Font&& other) noexcept = default;

cgss::Font::~Font() = default;

sf::Font& cgss::Font::raw() noexcept { return *m_font; }
const sf::Font& cgss::Font::raw() const noexcept { return *m_font; }
