// Raylib stub for cgss::FramedViewItem / cgss::FramedView. Selected by CMake
// when CGSS_BACKEND=raylib (folder-based per-backend specialization). All
// methods are no-op stubs — a real raylib impl would build on
// DrawTextureNPatch (raylib's built-in 9-slice draw) instead of composing
// per-section vertex arrays.
//
// The class layout (FramedViewItem holding `unique_ptr<sf::Sprite>`/
// `unique_ptr<sf::VertexArray>`) is preserved as forward-declared SFML types
// in FramedView.h — under raylib those members are default-constructed null
// pointers and never touched, just enough to satisfy the class layout.

// FramedView.h holds `unique_ptr<sf::Sprite>` / `vector<unique_ptr<sf::VertexArray>>`
// members (intentionally — those concrete types are unused at runtime under
// raylib but the class layout includes them). To synthesise the defaulted
// destructor we need the SFML types complete here. SFML headers are
// header-only C++; including them does NOT pull a link dependency on libsfml.
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "LiteCGSS/Views/FramedView.h"
#include "LiteCGSS/Views/LoggerViews.h"

cgss::FramedViewItem::FramedViewItem(FramedView*, MainEventDispatcher& eventDispatcher, DisplayWindowUserData windowData, const DisplayWindowSettings& windowSettings, std::weak_ptr<ViewportItem> parent) :
	ViewBox(eventDispatcher, windowSettings),
	SubObserver<ViewportChangeEvent>(std::bind(&FramedViewItem::onViewportChange, this, std::placeholders::_1), *this),
	m_windowData(std::move(windowData)),
	m_parent(std::move(parent)) {
}

cgss::FramedViewItem::~FramedViewItem() = default;

bool cgss::FramedViewItem::onViewportChange(cgss::ViewportChangeEvent&) { return true; }
void cgss::FramedViewItem::bindRectangleViewport(Rectangle*) {}
void cgss::FramedViewItem::draw(cgss::RenderTarget&) const {}
void cgss::FramedViewItem::drawFast(cgss::RenderTarget&) const {}
void cgss::FramedViewItem::drawCalculateView(ViewportItem&, cgss::Camera&) const {}
void cgss::FramedViewItem::update() {}
void cgss::FramedViewItem::onMove() {}
void cgss::FramedViewItem::onResize() {}
void cgss::FramedViewItem::updateCursorSprite() {}
void cgss::FramedViewItem::updatePauseSprite() {}
void cgss::FramedViewItem::resetPausePosition() {}
void cgss::FramedViewItem::updateContents() {}
void cgss::FramedViewItem::updateBackOpacity() {}
void cgss::FramedViewItem::updateContentsOpacity() {}
void cgss::FramedViewItem::updateVerticesBlt(long, long) {}
void cgss::FramedViewItem::updateVerticesStretch(long, long) {}
void cgss::FramedViewItem::allocateVerticesBlt(long, long, long, long) {}
void cgss::FramedViewItem::allocateVerticesStretch() {}
void cgss::FramedViewItem::calculateVertices(long, long, long, long, sf::Vector2i&, sf::IntRect&) {}
void cgss::FramedViewItem::calculateVerticesStretch(long, long, long, long, sf::Vector2i&, sf::Vector2i&, sf::IntRect&) {}
void cgss::FramedViewItem::updateVertices() {}
void cgss::FramedViewItem::updateView() {}

void cgss::FramedViewItem::setBuilder(std::array<long, 6> windowBuilder) {
	std::copy(std::begin(windowBuilder), std::begin(windowBuilder) + 6, std::begin(m_windowBuilder));
	m_windowBuilderExtended = false;
}
void cgss::FramedViewItem::setBuilder(std::array<long, 8> windowBuilder) {
	m_windowBuilder = std::move(windowBuilder);
	m_windowBuilderExtended = true;
}

long cgss::FramedViewItem::getPauseX() const { return m_pauseX.value_or(0); }
long cgss::FramedViewItem::getPauseY() const { return m_pauseY.value_or(0); }

void cgss::FramedViewItem::setBackOpacity(uint8_t backOpacity) { m_backOpacity = backOpacity; }
std::uint8_t cgss::FramedViewItem::getBackOpacity() const { return m_backOpacity; }

void cgss::FramedViewItem::lock()             { m_locked = true; }
void cgss::FramedViewItem::unlock()           { m_locked = false; }
bool cgss::FramedViewItem::isLocked() const   { return m_locked; }

cgss::IntRect cgss::FramedViewItem::getCursorRectangle() const { return m_cursorRect; }
void cgss::FramedViewItem::setCursorRectangle(cgss::IntRect rect) {
	setValue(rect);
	m_cursorRect = std::move(rect);
}

void cgss::FramedViewItem::setPauseSkin(cgss::Texture&) {}
void cgss::FramedViewItem::pause(bool pause) { m_paused = pause; }
void cgss::FramedViewItem::setPausePosition(float x, float y) {
	m_pauseX = static_cast<long>(x);
	m_pauseY = static_cast<long>(y);
}
bool cgss::FramedViewItem::isPaused() const { return m_paused; }

void cgss::FramedViewItem::setCursorSkin(cgss::Texture&) {}
void cgss::FramedViewItem::active(bool active)   { m_active = active; }
bool cgss::FramedViewItem::isActive() const      { return m_active; }

void cgss::FramedViewItem::stretch(bool stretch) { m_stretch = stretch; }
bool cgss::FramedViewItem::isStretched() const   { return m_stretch; }

void cgss::FramedViewItem::setOpacity(uint8_t opacity) { m_opacity = opacity; }
uint8_t cgss::FramedViewItem::getOpacity() const       { return m_opacity; }

void cgss::FramedViewItem::setContentsOpacity(uint8_t opacity) { m_contentOpacity = opacity; }
uint8_t cgss::FramedViewItem::getContentsOpacity() const       { return m_contentOpacity; }

void cgss::FramedViewItem::setSkin(cgss::Texture* skin) { m_texture = skin; }
cgss::Texture* cgss::FramedViewItem::getSkin() const    { return m_texture; }

void cgss::FramedViewItem::move(float x, float y) {
	m_x = x;
	m_y = y;
}
void cgss::FramedViewItem::updateFromValue(const cgss::IntRect* rectangle) {
	setCursorRectangle(rectangle == nullptr ? cgss::IntRect{} : *rectangle);
}

// ---- FramedView (proxy forwarder; identical between backends) ----

void cgss::FramedView::move(float x, float y)             { m_proxy->move(x, y); }
void cgss::FramedView::lock()                              { m_proxy->lock(); }
void cgss::FramedView::unlock()                            { m_proxy->unlock(); }
bool cgss::FramedView::isLocked() const                    { return m_proxy->isLocked(); }
void cgss::FramedView::setOpacity(uint8_t opacity)         { m_proxy->setOpacity(opacity); }
uint8_t cgss::FramedView::getOpacity() const               { return m_proxy->getOpacity(); }
void cgss::FramedView::setContentsOpacity(uint8_t opacity) { m_proxy->setContentsOpacity(opacity); }
uint8_t cgss::FramedView::getContentsOpacity() const       { return m_proxy->getContentsOpacity(); }
void cgss::FramedView::pause(bool pause)                   { m_proxy->pause(pause); }
bool cgss::FramedView::isPaused() const                    { return m_proxy->isPaused(); }
void cgss::FramedView::active(bool active)                 { m_proxy->active(active); }
bool cgss::FramedView::isActive() const                    { return m_proxy->isActive(); }
void cgss::FramedView::setSkin(cgss::Texture* skin)        { m_proxy->setSkin(skin); }
cgss::Texture* cgss::FramedView::getSkin() const           { return m_proxy->getSkin(); }
void cgss::FramedView::setPauseSkin(cgss::Texture& tex)    { m_proxy->setPauseSkin(tex); }
void cgss::FramedView::setPausePosition(float x, float y)  { m_proxy->setPausePosition(x, y); }
void cgss::FramedView::setCursorSkin(cgss::Texture& tex)   { m_proxy->setCursorSkin(tex); }
void cgss::FramedView::setCursorRectangle(cgss::IntRect r) { m_proxy->setCursorRectangle(std::move(r)); }
void cgss::FramedView::stretch(bool stretch)               { m_proxy->stretch(stretch); }
bool cgss::FramedView::isStretched() const                 { return m_proxy->isStretched(); }
void cgss::FramedView::draw(cgss::RenderTarget& target) const     { m_proxy->drawFast(target); }
void cgss::FramedView::drawFast(cgss::RenderTarget& target) const { m_proxy->draw(target); }
void cgss::FramedView::sortZ()                                    { m_proxy->sortZ(); }
void cgss::FramedView::bindRectangleViewport(Rectangle* output)   { m_proxy->bindRectangleViewport(output); }

void cgss::FramedView::moveOrigin(long ox, long oy) {
	m_proxy->m_wantedOx = ox;
	m_proxy->m_wantedOy = oy;
	m_proxy->moveOrigin(ox, oy);
}

void cgss::FramedView::setAngle(float angle) { m_proxy->setAngle(angle); }

void cgss::FramedView::resize(long width, long height) {
	m_proxy->m_wantedWidth = width;
	m_proxy->m_wantedHeight = height;
	m_proxy->resize(width, height);
}

void cgss::FramedView::setBuilder(std::array<long, 6> windowBuilder) { m_proxy->setBuilder(windowBuilder); }
void cgss::FramedView::setBuilder(std::array<long, 8> windowBuilder) { m_proxy->setBuilder(windowBuilder); }

long cgss::FramedView::getPauseX() const { return m_proxy->getPauseX(); }
long cgss::FramedView::getPauseY() const { return m_proxy->getPauseY(); }

cgss::IntRect cgss::FramedView::getCursorRectangle() const { return m_proxy->getCursorRectangle(); }
cgss::IntRect cgss::FramedView::getViewportBox() const     { return m_proxy->getRectangleBox(); }
float cgss::FramedView::getX() const      { return m_proxy->m_x; }
float cgss::FramedView::getY() const      { return m_proxy->m_y; }
float cgss::FramedView::getWidth() const  { return m_proxy->m_wantedWidth; }
float cgss::FramedView::getHeight() const { return m_proxy->m_wantedHeight; }
long cgss::FramedView::getOx() const      { return m_proxy->m_wantedOx; }
long cgss::FramedView::getOy() const      { return m_proxy->m_wantedOy; }

void cgss::FramedView::update() { m_proxy->update(); }
void cgss::FramedView::setBackOpacity(uint8_t backOpacity) { m_proxy->setBackOpacity(backOpacity); }
std::uint8_t cgss::FramedView::getBackOpacity() const      { return m_proxy->getBackOpacity(); }
