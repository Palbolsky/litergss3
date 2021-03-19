#include <cmath>
#include "LoggerDrawables.h"
#include "Text.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::TextItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::TextItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info, cgss::TextItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn, cgss::TextItem)

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

void cgss::TextItem::setOpacity(std::uint8_t opacity) {
	auto color = m_text.getFillColor();
	color.a = opacity;
	LOG_INFO << "Set opacity (" << static_cast<int>(opacity) << ")";
	m_text.setFillColor(color);
	if (m_text.getOutlineThickness() < 1.0f && opacity != 255) {
		color = m_text.getOutlineColor();
		color.a = opacity / 3;
		m_text.setOutlineColor(color);
	}
}

void cgss::Text::setOpacity(std::uint8_t opacity) {
	m_proxy->setOpacity(opacity);
}

float cgss::Text::getX() const {
	return m_proxy->m_wantedX;
}

float cgss::Text::getY() const {
	return m_proxy->m_wantedY;
}

float cgss::Text::getRealWidth() const {
	return m_proxy->m_text.getLocalBounds().width;
}

const sf::Color& cgss::Text::getFillColor() const {
	return m_proxy->m_text.getFillColor();
}

void cgss::Text::setFillColor(sf::Color color) {
	LOG_INFO << "Set fill color (r = " << static_cast<int>(color.r) << " g =" << static_cast<int>(color.g) << " b = " << static_cast<int>(color.b) << " a = " << static_cast<int>(color.a) << ")";
	m_proxy->m_text.setFillColor(color);
}

const sf::Color& cgss::Text::getOutlineColor() const {
	return m_proxy->m_text.getOutlineColor();
}

void cgss::Text::setOutlineColor(sf::Color color) {
	m_proxy->m_text.setOutlineColor(color);
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
	LOG_INFO << "Set style (" << style << ")";
	m_proxy->m_text.setStyle(style);
}

void cgss::Text::setDrawShadow(bool drawshadow) {
	m_proxy->m_text.setDrawShadow(drawshadow);
}

bool cgss::Text::getDrawShadow() const {
	return m_proxy->m_text.getDrawShadow();
}

void cgss::Text::setNumCharToDraw(sf::Uint32 num) {
	m_proxy->m_text.setNumCharToDraw(num);
	m_proxy->updateAlign();
}

sf::Uint32 cgss::Text::getNumCharToDraw() const {
	return m_proxy->m_text.getNumCharToDraw();
}

void cgss::Text::setLineHeight(float height) {
	m_proxy->m_text.setLineHeight(height);
}

sf::Uint32 cgss::Text::getTextWidth(const sf::String& string) const {
	return m_proxy->m_text.getTextWidth(string);
}

void cgss::Text::setSmooth(bool smooth) {
	m_proxy->m_text.setSmooth(smooth);
}

bool cgss::Text::getSmooth() const {
	return m_proxy->m_text.getSmooth();
}

void cgss::TextItem::updateAlign() {
	auto box = sf::FloatRect { m_wantedX, m_wantedY, static_cast<float>(m_width), static_cast<float>(m_height) };
	const auto localBounds = m_text.getLocalBounds();
	float ox = 0.f;
	switch (m_align) {
		case 1: /* Center */
			LOG_DEBUG << "Update align center";
			ox = std::round(localBounds.width / 2.f);
			box.left = std::round(box.left + box.width / 2.f);
			break;
		case 2: /* Right */
			LOG_DEBUG << "Update align right";
			ox = std::round(localBounds.width);
			box.left = std::round(box.left + box.width);
			break;
		default: /* Left */
			LOG_DEBUG << "Update align left";
			break;
	}
	box.top = std::round(box.top + box.height / 2);
	LOG_INFO << "Text position updated to x = " << box.left << " y = " << box.top;

	/* Position update */
	m_text.setPosition(box.left, box.top);
	m_text.setOrigin(ox, std::round(m_text.getCharacterSize() / 2.0f));
}

void cgss::Text::setAlign(uint8_t align) {
	m_proxy->m_align = align;
	m_proxy->updateAlign();
}

void cgss::Text::move(float x, float y) {
	m_proxy->m_wantedX = x;
	m_proxy->m_wantedY = y;
	LOG_INFO << "Text moved explicitely to x = " << x << " y = " << y;
	Transformable::move(x, y);
	m_proxy->updateAlign();
}

void cgss::Text::resize(long width, long height) {
	m_proxy->m_width = width;
	m_proxy->m_height = height;
	setLineHeight(height);
	LOG_INFO << "Text resized explicitely to width = " << width << " height = " << height;
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
	m_proxy->updateAlign();
}

void cgss::Text::setString(const sf::String& text) {
	LOG_INFO << "Set text string " << text.toAnsiString();
	m_proxy->m_text.setString(text);
	m_proxy->updateAlign();
}

void cgss::Text::setFont(const sf::Font& font) {
	LOG_INFO << "Set font (family = " << font.getInfo().family << ")";
	m_proxy->m_text.setFont(font);
}
