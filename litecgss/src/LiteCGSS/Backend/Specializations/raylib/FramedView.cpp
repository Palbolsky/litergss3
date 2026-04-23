// Raylib implementation of cgss::FramedViewItem / cgss::FramedView. Selected
// by CMake when CGSS_BACKEND=raylib (folder-based per-backend specialization).
//
// Scope: the minimum to render child drawables inside a framed region with
// the correct view transform (handled via the RaylibBackend view ops —
// set_view installs the scissored viewport automatically). The 9-slice skin
// (SFML uses vertex arrays; raylib's natural fit is DrawTextureNPatch) and
// the cursor/pause sprites are NOT yet drawn — they no-op when `m_texture`
// is null, which covers the common "unskinned framed view" case exercised
// by the playground. A real skin pass would populate m_impl with cached
// NPatchInfo / cursor ::Texture pointers and render them in drawFast.
//
// drawCalculateView (SFML's parent-viewport clipping math) is also skipped
// here: for the playground and any framed view whose rect fits inside its
// parent viewport, ViewBox::clone() produces a correct view. Nested views
// that extend outside their parent would benefit from reintroducing the
// same math used in the SFML specialization.

#include "LiteCGSS/Views/FramedView.h"
#include "LiteCGSS/Views/Viewport.h"
#include "LiteCGSS/Views/LoggerViews.h"
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Rectangle.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::FramedViewItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::FramedViewItem)
#define LOG_INFO  SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info,  cgss::FramedViewItem)
#define LOG_WARN  SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn,  cgss::FramedViewItem)

// Placeholder impl struct — populated by a future skin/cursor/pause pass.
struct cgss::detail::FramedViewImpl {
	// Reserved for future raylib-native skin state: NPatchInfo + cached
	// ::Texture pointers for the cursor and pause overlays.
};

cgss::FramedViewItem::FramedViewItem(FramedView*, MainEventDispatcher& eventDispatcher, DisplayWindowUserData windowData, const DisplayWindowSettings& windowSettings, std::weak_ptr<ViewportItem> parent) :
	ViewBox(eventDispatcher, windowSettings),
	SubObserver<ViewportChangeEvent>(std::bind(&FramedViewItem::onViewportChange, this, std::placeholders::_1), *this),
	m_windowData(std::move(windowData)),
	m_impl(std::make_unique<detail::FramedViewImpl>()),
	m_parent(std::move(parent)) {
}

cgss::FramedViewItem::~FramedViewItem() = default;

bool cgss::FramedViewItem::onViewportChange(cgss::ViewportChangeEvent& event) {
	if (m_bondRectViewport == nullptr) return true;
	m_bondRectViewport->setValue(event.viewportBox);
	return true;
}

void cgss::FramedViewItem::bindRectangleViewport(Rectangle* output) {
	m_bondRectViewport = output;
}

void cgss::FramedViewItem::draw(cgss::RenderTarget& target) const {
	drawFast(target);
}

void cgss::FramedViewItem::drawFast(cgss::RenderTarget& target) const {
	using Ops = cgss::backend::ActiveBackend::Ops;
	using cgss::backend::native_of;

	// Skin / cursor / pause draw passes not implemented yet — they require
	// the ported NPatchInfo-based 9-slice path. When m_texture is non-null
	// an unskinned framed view is still functional (just invisible outline).

	auto originalView = Ops::target_get_view(native_of(target));
	auto targetView = ViewBox::clone();

	const auto vp = Ops::view_get_viewport(native_of(targetView));
	if (vp.height > 0 && vp.width > 0) {
		Ops::target_set_view(native_of(target), native_of(targetView));
		drawables().drawFast(target);
	}
	Ops::target_set_view(native_of(target), originalView);
}

void cgss::FramedViewItem::drawCalculateView(ViewportItem& /*parent*/, cgss::Camera& /*targetView*/) const {
	// Parent-aware clipping is skipped in this backend — see file header.
}

void cgss::FramedViewItem::update() {
	updatePauseSprite();
	updateCursorSprite();
	m_counter = (m_counter + 1) % 128;
}

void cgss::FramedViewItem::onMove() {
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::onResize() {
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::updateVertices() {
	// 9-slice skin vertex rebuild — deferred to the skin/NPatch port.
	if (m_locked || m_wantedWidth <= 0 || m_wantedHeight <= 0) return;
	updateBackOpacity();
	resetPausePosition();
	updateContents();
}

void cgss::FramedViewItem::updateView() {
	const long offset_x  = m_windowBuilder[4];
	const long offset_y  = m_windowBuilder[5];
	const long offset_x2 = m_windowBuilderExtended ? m_windowBuilder[6] : offset_x;
	const long offset_y2 = m_windowBuilderExtended ? m_windowBuilder[7] : offset_y;
	const long x      = static_cast<long>(m_x) + offset_x;
	const long y      = static_cast<long>(m_y) + offset_y;
	long width        = static_cast<long>(m_wantedWidth)  - offset_x - offset_x2;
	long height       = static_cast<long>(m_wantedHeight) - offset_y - offset_y2;
	if (width  < 0) width  = 0;
	if (height < 0) height = 0;
	LOG_INFO << "View updated to x = " << x << " y = " << y << " width = " << width << " height = " << height;
	resize(width, height);
	moveViewport(static_cast<float>(x), static_cast<float>(y));
}

void cgss::FramedViewItem::move(float x, float y) {
	m_x = x;
	m_y = y;
	updateView();
}

// Cursor / pause state bookkeeping — animates but draws nothing until the
// skin path lands. Keeps opacity-driven state updates consistent with SFML.
void cgss::FramedViewItem::updatePauseSprite() {}
void cgss::FramedViewItem::updateCursorSprite() {}
void cgss::FramedViewItem::resetPausePosition() {}
void cgss::FramedViewItem::updateContents() {
	setCursorRectangle(m_cursorRect);
}
void cgss::FramedViewItem::updateBackOpacity() {}
void cgss::FramedViewItem::updateContentsOpacity() {
	const auto opacity = static_cast<uint8_t>((m_opacity * m_contentOpacity) / 255.f);
	drawables().updateContentsOpacity(opacity);
}
void cgss::FramedViewItem::updateVerticesBlt(long, long) {}
void cgss::FramedViewItem::updateVerticesStretch(long, long) {}
void cgss::FramedViewItem::allocateVerticesBlt(long, long, long, long) {}
void cgss::FramedViewItem::allocateVerticesStretch() {}

void cgss::FramedViewItem::setBuilder(std::array<long, 6> windowBuilder) {
	LOG_INFO << "Setting window builder 6";
	std::copy(std::begin(windowBuilder), std::begin(windowBuilder) + 6, std::begin(m_windowBuilder));
	m_windowBuilderExtended = false;
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::setBuilder(std::array<long, 8> windowBuilder) {
	LOG_INFO << "Setting window builder 8";
	m_windowBuilder = std::move(windowBuilder);
	m_windowBuilderExtended = true;
	updateVertices();
	updateView();
}

long cgss::FramedViewItem::getPauseX() const { return m_pauseX.value_or(0); }
long cgss::FramedViewItem::getPauseY() const { return m_pauseY.value_or(0); }

void cgss::FramedViewItem::setBackOpacity(uint8_t backOpacity) {
	m_backOpacity = backOpacity;
	updateBackOpacity();
}
std::uint8_t cgss::FramedViewItem::getBackOpacity() const { return m_backOpacity; }

void cgss::FramedViewItem::lock() {
	m_locked = true;
	LOG_INFO << "Locked";
}
void cgss::FramedViewItem::unlock() {
	m_locked = false;
	LOG_INFO << "Unlocked, updating vertices";
	updateVertices();
}
bool cgss::FramedViewItem::isLocked() const { return m_locked; }

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
void cgss::FramedViewItem::active(bool active) { m_active = active; }
bool cgss::FramedViewItem::isActive() const    { return m_active; }

void cgss::FramedViewItem::stretch(bool stretch) {
	m_stretch = stretch;
	updateVertices();
}
bool cgss::FramedViewItem::isStretched() const { return m_stretch; }

void cgss::FramedViewItem::setOpacity(uint8_t opacity) {
	m_opacity = opacity;
	updateBackOpacity();
	updateContentsOpacity();
}
uint8_t cgss::FramedViewItem::getOpacity() const { return m_opacity; }

void cgss::FramedViewItem::setContentsOpacity(uint8_t opacity) {
	m_contentOpacity = opacity;
	updateContentsOpacity();
}
uint8_t cgss::FramedViewItem::getContentsOpacity() const { return m_contentOpacity; }

void cgss::FramedViewItem::setSkin(cgss::Texture* skin) {
	m_texture = skin;
	LOG_INFO << "Skin set";
	updateVertices();
}
cgss::Texture* cgss::FramedViewItem::getSkin() const { return m_texture; }

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
