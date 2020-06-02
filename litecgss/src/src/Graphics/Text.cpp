#include <cmath>
#include "Text.h"

cgss::TextItem::TextItem(Text* owner) {
	owner->setInstance(m_text);
}

void cgss::TextItem::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void cgss::TextItem::drawFast(sf::RenderTarget& target) const {
	target.draw(m_text);
}

std::uint8_t cgss::Text::getOpacity() const {
	return m_proxy->m_text.getFillColor().a;
}

void cgss::Text::setOpacity(std::uint8_t opacity) {
	auto color = m_proxy->m_text.getFillColor();
	color.a = opacity;
	m_proxy->m_text.setFillColor(color);
	if (m_proxy->m_text.getOutlineThickness() < 1.0f && opacity != 255) {
		color = m_proxy->m_text.getOutlineColor();
		color.a = opacity / 3;
		m_proxy->m_text.setOutlineColor(std::move(color));
	}
}

float cgss::Text::getRealWidth() const {
	return m_proxy->m_text.getLocalBounds().width;
}

const sf::Color& cgss::Text::getFillColor() const {
	return m_proxy->m_text.getFillColor();
}

void cgss::Text::setFillColor(sf::Color color) {
	m_proxy->m_text.setFillColor(std::move(color));
}

const sf::Color& cgss::Text::getOutlineColor() const {
	return m_proxy->m_text.getOutlineColor();
}

void cgss::Text::setOutlineColor(sf::Color color) {
	m_proxy->m_text.setOutlineColor(std::move(color));
}

float cgss::Text::getOutlineThickness() const {
	return m_proxy->m_text.getOutlineThickness();
}

void cgss::Text::setOutlineThickness(float outlineThickness) {
	m_proxy->m_text.setOutlineThickness(outlineThickness);
}

sf::Uint32 cgss::Text::getStyle() const {
	return m_proxy->m_text.getStyle();
}

void cgss::Text::setStyle(sf::Uint32 style) {
	m_proxy->m_text.setStyle(style);
}

void cgss::TextItem::updateAlign() {
	auto box = sf::FloatRect { m_text.getPosition().x, m_text.getPosition().y, static_cast<float>(m_width), static_cast<float>(m_height) };
	auto bounds = m_text.getLocalBounds();
	float ox = 0.f;
	switch (m_align) {
		case 1: /* Center */
			ox = std::round(bounds.width / 2.f);
			box.left = std::round(box.left + box.width / 2.f);
			break;
		case 2: /* Right */
			ox = std::round(bounds.width);
			box.left = std::round(box.left + box.width);
			break;
		default: /* Left */
			break;
	}
	box.top = std::round(box.top + box.height / 2);
	/* Position update */
	m_text.setPosition(box.left, box.top);
	m_text.setOrigin(ox, std::round(m_text.getCharacterSize() / 2.0f));
}

void cgss::Text::setAlign(uint8_t align) {
	m_proxy->m_align = align;
	m_proxy->updateAlign();
}

void cgss::Text::move(float x, float y) {
	Transformable::move(x, y);
	m_proxy->updateAlign();
}

void cgss::Text::resize(long width, long height) {
	m_proxy->m_width = width;
	m_proxy->m_height = height;
	m_proxy->updateAlign();
}

long cgss::Text::getWidth() const {
	return m_proxy->m_width;
}

long cgss::Text::getHeight() const {
	return m_proxy->m_height;
}

long cgss::Text::getCharacterSize() const {
	return static_cast<long>(m_proxy->m_text.getCharacterSize());
}

void cgss::Text::setCharacterSize(long characterSize) {
	m_proxy->m_text.setCharacterSize(characterSize);
}

void cgss::Text::setString(const sf::String& text) {
	m_proxy->m_text.setString(text);
	m_proxy->updateAlign();
}

void cgss::Text::setFont(sf::Font& font) {
	m_proxy->m_text.setFont(font);
}
