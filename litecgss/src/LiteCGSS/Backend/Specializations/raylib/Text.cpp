// Raylib implementation of cgss::TextItem / cgss::Text. Selected by CMake
// when CGSS_BACKEND=raylib (folder-based per-backend specialization). The
// rendering engine is DrawTextPro against a raylib ::Font atlas — see the
// sibling ScrollText.h for the state object held by the pimpl.
//
// `cgss::TextItem` holds an opaque `std::unique_ptr<ScrollText> m_text` so
// the public Text.h header does not pull ScrollText internals. The raylib
// ScrollText's internal API diverges from the SFML-vendored ScrollText
// (raylib uses UTF-8 strings + ::Color/::Font/::Rectangle); each backend's
// Text.cpp is free to diverge since neither sees the other's ScrollText.
//
// Phase 1 coverage: string, font, character size, fill color, position /
// origin / scale, numCharsToDraw (typewriter), smoothing, text width
// measurement. Outline, drop shadow, Bold/Italic/Underline/StrikeThrough
// style flags are accepted as setters but not yet rendered — tracked for
// Phase 2. Kerning and per-glyph bounds refinement (findCharacterPos,
// true ascender/descender bounds) tracked for Phase 3.

#include <cmath>

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Backend/Specializations/raylib/ScrollText.h"
#include "LiteCGSS/Graphics/LoggerDrawables.h"
#include "LiteCGSS/Graphics/Text.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::TextItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::TextItem)
#define LOG_INFO  SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info,  cgss::TextItem)
#define LOG_WARN  SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn,  cgss::TextItem)

namespace {
	constexpr ::Color toRaylib(cgss::Color c) noexcept {
		return ::Color{ c.r, c.g, c.b, c.a };
	}
	constexpr cgss::Color fromRaylib(::Color c) noexcept {
		return cgss::Color{ c.r, c.g, c.b, c.a };
	}
}

cgss::TextItem::TextItem(Text*) :
	m_text(std::make_unique<ScrollText>()) {
}

cgss::TextItem::~TextItem() = default;

void cgss::TextItem::draw(cgss::RenderTarget& target) const {
	drawFast(target);
}

void cgss::TextItem::drawFast(cgss::RenderTarget& /*target*/) const {
	// Raylib draw ops are target-implicit — they render against the
	// currently-active BeginDrawing / BeginTextureMode scope set up by
	// the surrounding Viewport / DisplayWindow pass. Matches the pattern
	// used by RaylibBackend::Ops::draw_sprite / draw_shape.
	m_text->draw();
}

std::uint8_t cgss::Text::getOpacity() const {
	return m_proxy->m_text->getFillColor().a;
}

void cgss::TextItem::setOpacity(std::uint8_t opacity) {
	auto fill = m_text->getFillColor();
	fill.a = opacity;
	LOG_INFO << "Set opacity (" << static_cast<int>(opacity) << ")";
	m_text->setFillColor(fill);
	// Mirrors the SFML variant: when no real outline is drawn, the
	// outline color follows the fill's alpha (scaled down) so the future
	// Phase-2 outline/shadow pass picks up opacity transitions.
	if (m_text->getOutlineThickness() < 1.0f) {
		auto outline = m_text->getOutlineColor();
		outline.a = opacity;
		if (opacity != 255) outline.a /= 3;
		m_text->setOutlineColor(outline);
	}
}

void cgss::Text::setOpacity(std::uint8_t opacity) {
	m_proxy->setOpacity(opacity);
}

float cgss::Text::getX() const { return m_proxy->m_wantedX; }
float cgss::Text::getY() const { return m_proxy->m_wantedY; }

float cgss::Text::getRealWidth() const {
	return m_proxy->m_text->getLocalBounds().width;
}

float cgss::Text::getRealHeight() const {
	return m_proxy->m_text->getLocalBounds().height;
}

cgss::Color cgss::Text::getFillColor() const {
	return fromRaylib(m_proxy->m_text->getFillColor());
}

void cgss::Text::setFillColor(cgss::Color color) {
	LOG_INFO << "Set fill color (r=" << static_cast<int>(color.r) << " g=" << static_cast<int>(color.g) << " b=" << static_cast<int>(color.b) << " a=" << static_cast<int>(color.a) << ")";
	m_proxy->m_text->setFillColor(toRaylib(color));
}

cgss::Color cgss::Text::getOutlineColor() const {
	return fromRaylib(m_proxy->m_text->getOutlineColor());
}

void cgss::Text::setOutlineColor(cgss::Color color) {
	m_proxy->m_text->setOutlineColor(toRaylib(color));
}

float cgss::Text::getOutlineThickness() const {
	return m_proxy->m_text->getOutlineThickness();
}

void cgss::Text::setOutlineThickness(float outlineThickness) {
	m_proxy->m_text->setOutlineThickness(outlineThickness);
}

std::uint32_t cgss::Text::getStyle() const {
	return m_proxy->m_text->getStyle();
}

void cgss::Text::setStyle(std::uint32_t style) {
	LOG_INFO << "Set style (" << style << ")";
	m_proxy->m_text->setStyle(style);
}

void cgss::Text::setDrawShadow(bool drawshadow) {
	m_proxy->m_text->setDrawShadow(drawshadow);
}

bool cgss::Text::getDrawShadow() const {
	return m_proxy->m_text->getDrawShadow();
}

void cgss::Text::setNumCharToDraw(std::uint32_t num) {
	m_proxy->m_text->setNumCharToDraw(num);
	m_proxy->updateAlign();
}

std::uint32_t cgss::Text::getNumCharToDraw() const {
	return m_proxy->m_text->getNumCharToDraw();
}

void cgss::Text::setLineHeight(float height) {
	m_proxy->m_text->setLineHeight(height);
}

std::uint32_t cgss::Text::getTextWidth(const std::string& string) const {
	return m_proxy->m_text->getTextWidth(string);
}

void cgss::Text::setSmooth(bool smooth) {
	m_proxy->m_text->setSmooth(smooth);
}

bool cgss::Text::getSmooth() const {
	return m_proxy->m_text->getSmooth();
}

void cgss::TextItem::updateAlign() {
	const auto bounds = m_text->getLocalBounds();
	float boxLeft  = m_wantedX;
	float boxTop   = m_wantedY;
	const float boxWidth  = static_cast<float>(m_width);
	const float boxHeight = static_cast<float>(m_height);
	float ox = 0.f;
	switch (m_align) {
		case 1: /* Center */
			LOG_DEBUG << "Update align center";
			ox = std::round(bounds.width / 2.f);
			boxLeft = std::round(boxLeft + boxWidth / 2.f);
			break;
		case 2: /* Right */
			LOG_DEBUG << "Update align right";
			ox = std::round(bounds.width);
			boxLeft = std::round(boxLeft + boxWidth);
			break;
		default: /* Left */
			LOG_DEBUG << "Update align left";
			break;
	}
	boxTop = std::round(boxTop + boxHeight / 2.f);
	LOG_INFO << "Text position updated to x=" << boxLeft << " y=" << boxTop;

	m_text->setPosition(boxLeft, boxTop);
	m_text->setOrigin(ox, std::round(m_text->getCharacterSize() / 2.0f));
}

void cgss::Text::setAlign(uint8_t align) {
	m_proxy->m_align = align;
	m_proxy->updateAlign();
}

void cgss::Text::move(float x, float y) {
	m_proxy->m_wantedX = x;
	m_proxy->m_wantedY = y;
	LOG_INFO << "Text moved explicitely to x=" << x << " y=" << y;
	Transformable::move(x, y);
	m_proxy->updateAlign();
}

void cgss::Text::scale(float xScale, float yScale) {
	Transformable::scale(xScale, yScale);
	m_proxy->m_text->setScale(xScale, yScale);
}

void cgss::Text::resize(long width, long height) {
	m_proxy->m_width  = width;
	m_proxy->m_height = height;
	setLineHeight(static_cast<float>(height));
	LOG_INFO << "Text resized explicitely to width=" << width << " height=" << height;
	m_proxy->updateAlign();
}

long cgss::Text::getWidth()  const { return m_proxy->m_width; }
long cgss::Text::getHeight() const { return m_proxy->m_height; }

long cgss::Text::getCharacterSize() const {
	return static_cast<long>(m_proxy->m_text->getCharacterSize());
}

void cgss::Text::setCharacterSize(long characterSize) {
	m_proxy->m_text->setCharacterSize(static_cast<unsigned int>(characterSize));
	m_proxy->updateAlign();
}

void cgss::Text::setString(const std::string& text) {
	LOG_INFO << "Set text string " << text;
	m_proxy->m_text->setString(text);
	m_proxy->updateAlign();
}

void cgss::Text::setFont(const cgss::Font& font) {
	m_proxy->m_text->setFont(cgss::backend::native_of(font));
}
