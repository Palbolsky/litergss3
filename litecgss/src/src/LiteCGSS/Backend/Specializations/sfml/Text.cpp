// SFML implementation of cgss::TextItem / cgss::Text — selected by CMake
// when CGSS_BACKEND=sfml (folder-based per-backend specialization). The
// raylib equivalent lives in Backend/Specializations/raylib/Text.cpp and
// provides stub no-op bodies for the same class methods.
//
// Per existing project policy (memory: SFML isolation effort), the
// underlying ScrollText is intentionally SFML-coupled — its source stays
// in Graphics/ScrollText.{h,cpp} since it's effectively a vendored copy
// of sf::Text. A future raylib-native text implementation would build on
// raylib's LoadFontFromMemory + DrawTextEx with its own glyph atlas.

#include <cmath>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/String.hpp>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Internal/SfColorConv.h"
#include "LiteCGSS/Graphics/LoggerDrawables.h"
#include "LiteCGSS/Graphics/Text.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::TextItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::TextItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info, cgss::TextItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn, cgss::TextItem)

cgss::TextItem::TextItem(Text*) {
}

void cgss::TextItem::draw(cgss::RenderTarget& target) const {
	drawFast(target);
}

void cgss::TextItem::drawFast(cgss::RenderTarget& target) const {
	cgss::backend::native_of(target).draw(m_text);
}

std::uint8_t cgss::Text::getOpacity() const {
	return m_proxy->m_text.getFillColor().a;
}

void cgss::TextItem::setOpacity(std::uint8_t opacity) {
	auto color = m_text.getFillColor();
	color.a = opacity;
	LOG_INFO << "Set opacity (" << static_cast<int>(opacity) << ")";
	m_text.setFillColor(color);
	if (m_text.getOutlineThickness() < 1.0f) {
		color = m_text.getOutlineColor();
    color.a = opacity;
    if (opacity != 255) {
		color.a /= 3;
    }
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

float cgss::Text::getRealHeight() const {
	return m_proxy->m_text.getLocalBounds().height;
}

cgss::Color cgss::Text::getFillColor() const {
	return cgss::internal::fromSf(m_proxy->m_text.getFillColor());
}

void cgss::Text::setFillColor(cgss::Color color) {
	LOG_INFO << "Set fill color (r = " << static_cast<int>(color.r) << " g =" << static_cast<int>(color.g) << " b = " << static_cast<int>(color.b) << " a = " << static_cast<int>(color.a) << ")";
	m_proxy->m_text.setFillColor(cgss::internal::toSf(color));
}

cgss::Color cgss::Text::getOutlineColor() const {
	return cgss::internal::fromSf(m_proxy->m_text.getOutlineColor());
}

void cgss::Text::setOutlineColor(cgss::Color color) {
	m_proxy->m_text.setOutlineColor(cgss::internal::toSf(color));
}

float cgss::Text::getOutlineThickness() const {
	return m_proxy->m_text.getOutlineThickness();
}

void cgss::Text::setOutlineThickness(float outlineThickness) {
	m_proxy->m_text.setOutlineThickness(outlineThickness);
}

std::uint32_t cgss::Text::getStyle() const {
	return m_proxy->m_text.getStyle();
}

void cgss::Text::setStyle(std::uint32_t style) {
	LOG_INFO << "Set style (" << style << ")";
	m_proxy->m_text.setStyle(style);
}

void cgss::Text::setDrawShadow(bool drawshadow) {
	m_proxy->m_text.setDrawShadow(drawshadow);
}

bool cgss::Text::getDrawShadow() const {
	return m_proxy->m_text.getDrawShadow();
}

void cgss::Text::setNumCharToDraw(std::uint32_t num) {
	m_proxy->m_text.setNumCharToDraw(num);
	m_proxy->updateAlign();
}

std::uint32_t cgss::Text::getNumCharToDraw() const {
	return m_proxy->m_text.getNumCharToDraw();
}

void cgss::Text::setLineHeight(float height) {
	m_proxy->m_text.setLineHeight(height);
}

std::uint32_t cgss::Text::getTextWidth(const std::string& string) const {
	return m_proxy->m_text.getTextWidth(sf::String::fromUtf8(string.begin(), string.end()));
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

void cgss::Text::scale(float xScale, float yScale) {
	Transformable::scale(xScale, yScale);
	m_proxy->m_text.setScale(xScale, yScale);
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

void cgss::Text::setString(const std::string& text) {
	LOG_INFO << "Set text string " << text;
	m_proxy->m_text.setString(sf::String::fromUtf8(text.begin(), text.end()));
	m_proxy->updateAlign();
}

void cgss::Text::setFont(const cgss::Font& font) {
	LOG_INFO << "Set font (family = " << cgss::backend::native_of(font).getInfo().family << ")";
	m_proxy->m_text.setFont(cgss::backend::native_of(font));
}
