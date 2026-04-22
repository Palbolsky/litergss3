// Raylib stub for cgss::TextItem / cgss::Text. Selected by CMake when
// CGSS_BACKEND=raylib (folder-based per-backend specialization). All
// methods are no-op stubs — a real raylib text impl would build on
// LoadFontFromMemory + DrawTextEx with its own glyph atlas, replacing
// the SFML-vendored ScrollText that the SFML impl uses.
//
// Note that `cgss::TextItem` has a `ScrollText m_text` member declared
// in Graphics/Text.h. ScrollText.h/.cpp stay in Graphics/ (they're an
// SFML-vendored class) and compile under raylib too, but their symbols
// are unused — we just need the default-constructed `m_text` to satisfy
// the class layout.

#include "LiteCGSS/Graphics/Text.h"

cgss::TextItem::TextItem(Text*) {}

void cgss::TextItem::draw(cgss::RenderTarget&) const {}
void cgss::TextItem::drawFast(cgss::RenderTarget&) const {}
void cgss::TextItem::setOpacity(std::uint8_t) {}
void cgss::TextItem::updateAlign() {}

std::uint8_t cgss::Text::getOpacity() const { return 255; }
void  cgss::Text::setOpacity(std::uint8_t) {}
float cgss::Text::getX() const { return m_proxy->m_wantedX; }
float cgss::Text::getY() const { return m_proxy->m_wantedY; }
float cgss::Text::getRealWidth()  const { return 0.f; }
float cgss::Text::getRealHeight() const { return 0.f; }
cgss::Color cgss::Text::getFillColor() const { return cgss::Colors::White; }
void cgss::Text::setFillColor(cgss::Color) {}
cgss::Color cgss::Text::getOutlineColor() const { return cgss::Colors::Black; }
void cgss::Text::setOutlineColor(cgss::Color) {}
float cgss::Text::getOutlineThickness() const { return 0.f; }
void  cgss::Text::setOutlineThickness(float) {}
std::uint32_t cgss::Text::getStyle() const { return 0; }
void  cgss::Text::setStyle(std::uint32_t) {}
void  cgss::Text::setDrawShadow(bool) {}
bool  cgss::Text::getDrawShadow() const { return false; }
void  cgss::Text::setNumCharToDraw(std::uint32_t) {}
std::uint32_t cgss::Text::getNumCharToDraw() const { return 0; }
void  cgss::Text::setLineHeight(float) {}
std::uint32_t cgss::Text::getTextWidth(const std::string&) const { return 0; }
void  cgss::Text::setSmooth(bool) {}
bool  cgss::Text::getSmooth() const { return false; }
void  cgss::Text::setAlign(std::uint8_t) {}
void  cgss::Text::move(float x, float y) {
	m_proxy->m_wantedX = x;
	m_proxy->m_wantedY = y;
	Transformable::move(x, y);
}
void  cgss::Text::scale(float xScale, float yScale) {
	Transformable::scale(xScale, yScale);
}
void  cgss::Text::resize(long width, long height) {
	m_proxy->m_width  = width;
	m_proxy->m_height = height;
}
long  cgss::Text::getWidth()  const { return m_proxy->m_width; }
long  cgss::Text::getHeight() const { return m_proxy->m_height; }
long  cgss::Text::getCharacterSize() const { return 0; }
void  cgss::Text::setCharacterSize(long) {}
void  cgss::Text::setString(const std::string&) {}
void  cgss::Text::setFont(const cgss::Font&) {}
