// Backend-agnostic body for cgss::FramedViewItem / cgss::FramedView.
//
// 9-slice skin + cursor + pause draws are implemented via Ops::draw_texture_pro
// (≤9 calls per frame, all going through the active render target). The
// per-frame state churn is negligible at the FramedView usage rate (1-3
// message windows on screen), so we trade vertex batching for a single
// shared, backend-neutral `.cpp`; geometry is rebuilt on every drawFast.
#include <algorithm>
#include <cstdint>
#include "Views/LoggerViews.h"
#include "LiteCGSS/Views/FramedView.h"
#include "LiteCGSS/Views/Viewport.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Rectangle.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::FramedViewItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::FramedViewItem)
#define LOG_INFO  SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info,  cgss::FramedViewItem)
#define LOG_WARN  SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn,  cgss::FramedViewItem)

// Backend state shared by both backends — just two non-owning skin handles.
// The 9-slice skin, cursor + pause overlays are re-derived from the public
// FramedView state on every drawFast (cheap; ≤9 draw calls).
struct cgss::detail::FramedViewImpl {
	const cgss::Texture* cursorSkin = nullptr;
	const cgss::Texture* pauseSkin  = nullptr;
};

namespace {
	using Ops = cgss::backend::ActiveBackend::Ops;
	using cgss::backend::native_of;

	// One textured rect onto the bound target. `tint.a` carries the combined
	// (m_opacity * m_backOpacity) alpha and (m_opacity) alpha for cursor/pause.
	inline void blit(cgss::RenderTarget& target, const cgss::Texture& tex,
	                 long sx, long sy, long sw, long sh,
	                 float dx, float dy, float dw, float dh, cgss::Color tint) {
		if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;
		Ops::draw_texture_pro(native_of(target), native_of(tex),
		                      cgss::IntRect{ static_cast<int>(sx), static_cast<int>(sy),
		                                     static_cast<int>(sw), static_cast<int>(sh) },
		                      dx, dy, dw, dh,
		                      0.f, 0.f, 0.f, tint);
	}

	// Tile a single source rect across [dstX..dstX+dstW] horizontally, drawing
	// `srcW`-wide strips and clipping the trailing remainder to fit. Used for
	// the four edges and the middle in blt (non-stretch) mode.
	inline void blit_tiled_h(cgss::RenderTarget& target, const cgss::Texture& tex,
	                         long sx, long sy, long sw, long sh,
	                         float dstX, float dstY, float dstW, cgss::Color tint) {
		if (sw <= 0 || sh <= 0 || dstW <= 0) return;
		float x = dstX;
		const float endX = dstX + dstW;
		while (x + sw <= endX) {
			blit(target, tex, sx, sy, sw, sh, x, dstY, static_cast<float>(sw), static_cast<float>(sh), tint);
			x += sw;
		}
		const float remainder = endX - x;
		if (remainder > 0.f) {
			blit(target, tex, sx, sy, static_cast<long>(remainder), sh,
			     x, dstY, remainder, static_cast<float>(sh), tint);
		}
	}

	// Vertical sibling of blit_tiled_h.
	inline void blit_tiled_v(cgss::RenderTarget& target, const cgss::Texture& tex,
	                         long sx, long sy, long sw, long sh,
	                         float dstX, float dstY, float dstH, cgss::Color tint) {
		if (sw <= 0 || sh <= 0 || dstH <= 0) return;
		float y = dstY;
		const float endY = dstY + dstH;
		while (y + sh <= endY) {
			blit(target, tex, sx, sy, sw, sh, dstX, y, static_cast<float>(sw), static_cast<float>(sh), tint);
			y += sh;
		}
		const float remainder = endY - y;
		if (remainder > 0.f) {
			blit(target, tex, sx, sy, sw, static_cast<long>(remainder),
			     dstX, y, static_cast<float>(sw), remainder, tint);
		}
	}

	// 2-D tile of the middle slice. blt_tiled_h handles the trailing
	// horizontal remainder per row; we step rows by sh and clip the final
	// row's height.
	inline void blit_tiled_hv(cgss::RenderTarget& target, const cgss::Texture& tex,
	                          long sx, long sy, long sw, long sh,
	                          float dstX, float dstY, float dstW, float dstH, cgss::Color tint) {
		if (sw <= 0 || sh <= 0 || dstW <= 0 || dstH <= 0) return;
		float y = dstY;
		const float endY = dstY + dstH;
		while (y + sh <= endY) {
			blit_tiled_h(target, tex, sx, sy, sw, sh, dstX, y, dstW, tint);
			y += sh;
		}
		const float remainder = endY - y;
		if (remainder > 0.f) {
			// Clip the final row to the remaining height; horizontal
			// clipping is delegated to blit_tiled_h.
			blit_tiled_h(target, tex, sx, sy, sw, static_cast<long>(remainder), dstX, y, dstW, tint);
		}
	}
}

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

// Draw order:
//   1. Skin (current/parent view)
//   2. Children (inner view = ViewBox::clone, optionally clipped to parent)
//   3. Cursor (inner view — cursor coords are local to the dialog)
//   4. Pause (current/parent view — uses absolute screen coords)
void cgss::FramedViewItem::drawFast(cgss::RenderTarget& target) const {
	const auto originalView = Ops::target_get_view(native_of(target));

	// 1) Skin pass.
	if (m_texture != nullptr && m_wantedWidth > 0 && m_wantedHeight > 0) {
		const auto texSize = m_texture->getSize();
		const long ws = texSize.x;
		const long hs = texSize.y;
		const long xm = m_windowBuilder[0];
		const long ym = m_windowBuilder[1];
		const long wm = m_windowBuilder[2];
		const long hm = m_windowBuilder[3];

		// Corner sizes — see ASCII grid in updateVertices documentation.
		long w1 = xm;
		long w3 = ws - xm - wm;
		long delta_w = static_cast<long>(m_wantedWidth) - w1 - w3;
		if (delta_w < 0) {
			// The middle is gone; collapse the two corners proportionally so
			// they don't overlap past the window's right edge.
			delta_w /= 2;
			w1 += delta_w;
			w3 += delta_w;
			delta_w = static_cast<long>(m_wantedWidth) - w1 - w3;
		}
		long h1 = ym;
		long h7 = hs - ym - hm;
		long delta_h = static_cast<long>(m_wantedHeight) - h1 - h7;
		if (delta_h < 0) {
			delta_h /= 2;
			h1 += delta_h;
			h7 += delta_h;
			delta_h = static_cast<long>(m_wantedHeight) - h1 - h7;
		}

		const float x = m_x;
		const float y = m_y;
		const float wt = static_cast<float>(m_wantedWidth);
		const float ht = static_cast<float>(m_wantedHeight);
		const cgss::Color tint{ 255, 255, 255,
		                        static_cast<uint8_t>((m_opacity * m_backOpacity) / 255) };

		// Source coordinates of the 9 slices on the windowskin.
		const long sx_l = 0,        sx_m = xm,  sx_r = xm + wm;
		const long sy_t = 0,        sy_m = ym,  sy_b = ym + hm;
		const long sw_l = std::max<long>(w1, 0);
		const long sw_r = std::max<long>(w3, 0);
		const long sh_t = std::max<long>(h1, 0);
		const long sh_b = std::max<long>(h7, 0);

		// Destination coordinates within the framed window.
		const float dx_l = x,                 dx_r = x + wt - sw_r;
		const float dy_t = y,                 dy_b = y + ht - sh_b;
		const float dx_mid = x + sw_l;
		const float dy_mid = y + sh_t;
		const float dw_mid = std::max(0.f, wt - static_cast<float>(sw_l + sw_r));
		const float dh_mid = std::max(0.f, ht - static_cast<float>(sh_t + sh_b));

		// Corners — always 1:1 source→dest. delta_{w,h} ≤ 0 collapses the
		// middle to a point, which we still want behind the corners.
		blit(target, *m_texture, sx_l, sy_t, sw_l, sh_t, dx_l,   dy_t,   sw_l, sh_t, tint);
		blit(target, *m_texture, sx_r, sy_t, sw_r, sh_t, dx_r,   dy_t,   sw_r, sh_t, tint);
		blit(target, *m_texture, sx_l, sy_b, sw_l, sh_b, dx_l,   dy_b,   sw_l, sh_b, tint);
		blit(target, *m_texture, sx_r, sy_b, sw_r, sh_b, dx_r,   dy_b,   sw_r, sh_b, tint);

		if (m_stretch) {
			// One scaled draw per side / middle — matches RGSS's stretch
			// flag (smooth gradient skins).
			blit(target, *m_texture, sx_m, sy_t, wm, sh_t, dx_mid, dy_t,   dw_mid, sh_t,   tint); // top
			blit(target, *m_texture, sx_m, sy_b, wm, sh_b, dx_mid, dy_b,   dw_mid, sh_b,   tint); // bottom
			blit(target, *m_texture, sx_l, sy_m, sw_l, hm, dx_l,   dy_mid, sw_l,   dh_mid, tint); // left
			blit(target, *m_texture, sx_r, sy_m, sw_r, hm, dx_r,   dy_mid, sw_r,   dh_mid, tint); // right
			blit(target, *m_texture, sx_m, sy_m, wm, hm,   dx_mid, dy_mid, dw_mid, dh_mid, tint); // center
		} else {
			// Tile mode (RGSS default): repeat the middle source rect to fill.
			blit_tiled_h(target,  *m_texture, sx_m, sy_t, wm, sh_t, dx_mid, dy_t,   dw_mid, tint);
			blit_tiled_h(target,  *m_texture, sx_m, sy_b, wm, sh_b, dx_mid, dy_b,   dw_mid, tint);
			blit_tiled_v(target,  *m_texture, sx_l, sy_m, sw_l, hm, dx_l,   dy_mid, dh_mid, tint);
			blit_tiled_v(target,  *m_texture, sx_r, sy_m, sw_r, hm, dx_r,   dy_mid, dh_mid, tint);
			blit_tiled_hv(target, *m_texture, sx_m, sy_m, wm, hm,   dx_mid, dy_mid, dw_mid, dh_mid, tint);
		}
	}

	// 2) Children — switch to the inner view (clipped to parent if any).
	auto targetView = ViewBox::clone();
	auto parentView = m_parent.lock();
	if (parentView != nullptr) {
		drawCalculateView(*parentView, targetView);
	}

	const auto vp = Ops::view_get_viewport(native_of(targetView));
	if (vp.height > 0 && vp.width > 0) {
		Ops::target_set_view(native_of(target), native_of(targetView));
		drawables().drawFast(target);

		// 3) Cursor — drawn IN the inner view so cursor_rect coords are local.
		if (m_impl->cursorSkin != nullptr && m_cursorRect.width > 0 && m_cursorRect.height > 0) {
			const auto cs = m_impl->cursorSkin->getSize();
			// Animated alpha — 0..63 = fade in, 64..127 = fade out (active),
			// or steady 128 when inactive (historical cursor blink behavior).
			const long v = m_counter;
			uint8_t a;
			if (v > 64) {
				a = static_cast<uint8_t>(m_active ? 128 + 2 * (v - 64) : 128);
			} else {
				a = static_cast<uint8_t>(m_active ? 255 - 2 * v : 128);
			}
			a = static_cast<uint8_t>((a * m_opacity) / 255);
			const cgss::Color tint{ 255, 255, 255, a };
			blit(target, *m_impl->cursorSkin,
			     0, 0, static_cast<long>(cs.x), static_cast<long>(cs.y),
			     static_cast<float>(m_cursorRect.left),  static_cast<float>(m_cursorRect.top),
			     static_cast<float>(m_cursorRect.width), static_cast<float>(m_cursorRect.height),
			     tint);
		}
	}
	Ops::target_set_view(native_of(target), originalView);

	// 4) Pause — drawn in the original view at absolute screen coords.
	if (m_paused && m_impl->pauseSkin != nullptr) {
		const auto ps = m_impl->pauseSkin->getSize();
		// Each frame is half-size on both axes (2x2 sheet). v cycles 0..7.
		const long v = m_counter / 16;
		const long fw = ps.x / 2;
		const long fh = ps.y / 2;
		const long sx = (v & 0x01) ? fw : 0;
		const long sy = (v & 0x02) ? fh : 0;
		const float px = m_pauseX.has_value()
			? m_x + static_cast<float>(m_pauseX.value())
			: m_x + (m_wantedWidth - fw) / 2.f;
		const float py = m_pauseY.has_value()
			? m_y + static_cast<float>(m_pauseY.value())
			: m_y + (m_wantedHeight - fh) / 2.f;
		const cgss::Color tint{ 255, 255, 255, m_opacity };
		blit(target, *m_impl->pauseSkin, sx, sy, fw, fh,
		     px, py, static_cast<float>(fw), static_cast<float>(fh), tint);
	}
}

// Parent-aware clipping: if this FramedView is inside a Viewport, the
// inner view must be intersected with the parent's viewport box so children
// don't render outside the parent. Pure math + Ops::view_*; no backend
// native types involved.
void cgss::FramedViewItem::drawCalculateView(ViewportItem& parent, cgss::Camera& targetView) const {
	const cgss::IntRect parentViewport = parent.getViewportBox();
	const float zoom = parent.getZoom();

	const auto vx0 = parentViewport.width  / 2.f - (parentViewport.width  / 2.f) / zoom;
	const auto vy0 = parentViewport.height / 2.f - (parentViewport.height / 2.f) / zoom;

	auto targetCenter   = Ops::view_get_center(native_of(targetView));
	auto targetSize     = Ops::view_get_size(native_of(targetView));
	auto targetViewport = Ops::view_get_viewport(native_of(targetView));

	const auto sw = static_cast<float>(ViewBox::getScreenWidth());
	const auto sh = static_cast<float>(ViewBox::getScreenHeight());

	// Convert the (normalized) viewport rect into pixel coords.
	targetViewport.left   *= sw;
	targetViewport.top    *= sh;
	targetViewport.width  *= sw;
	targetViewport.height *= sh;

	// Apply the parent's origin offset.
	targetViewport.left -= parent.getOx();
	targetViewport.top  -= parent.getOy();

	// Apply the parent's zoom.
	targetViewport.left   /= zoom;
	targetViewport.top    /= zoom;
	targetViewport.width  /= zoom;
	targetViewport.height /= zoom;

	targetViewport.left += vx0;
	targetViewport.top  += vy0;

	// Translate into the parent's coordinate frame.
	targetViewport.left += parentViewport.left;
	targetViewport.top  += parentViewport.top;

	// Clip top-left against the parent's top-left.
	float dy = parentViewport.top - targetViewport.top;
	if (dy > 0) {
		targetViewport.top    += dy;
		targetSize.y          -= dy * zoom;
		targetViewport.height -= dy;
		targetCenter.y         = dy * zoom + targetViewport.height * zoom / 2.f;
	}
	float dx = parentViewport.left - targetViewport.left;
	if (dx > 0) {
		targetViewport.left  += dx;
		targetSize.x         -= dx * zoom;
		targetViewport.width -= dx;
		targetCenter.x        = dx * zoom + targetViewport.width * zoom / 2.f;
	}

	// Clip bottom-right against the parent's bottom-right.
	dy = (targetViewport.top + targetViewport.height) - (parentViewport.top + parentViewport.height);
	if (dy > 0) {
		targetViewport.height -= dy;
		targetSize.y          -= dy * zoom;
		targetCenter.y        -= dy * zoom / 2.f;
	}
	dx = (targetViewport.left + targetViewport.width) - (parentViewport.left + parentViewport.width);
	if (dx > 0) {
		targetViewport.width -= dx;
		targetSize.x         -= dx * zoom;
		targetCenter.x       -= dx * zoom / 2.f;
	}

	Ops::view_set_center(native_of(targetView), targetCenter.x, targetCenter.y);
	Ops::view_set_size(native_of(targetView), targetSize.x, targetSize.y);
	Ops::view_set_viewport(native_of(targetView),
		targetViewport.left / sw, targetViewport.top / sh,
		targetViewport.width / sw, targetViewport.height / sh);
}

void cgss::FramedViewItem::update() {
	m_counter = (m_counter + 1) % 128;
}

void cgss::FramedViewItem::updateVertices() {
	if (m_locked || m_wantedWidth <= 0 || m_wantedHeight <= 0) return;
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

// Animation/positioning is now derived per-frame in drawFast; these hooks
// are kept (header-declared) but have no work to do.
void cgss::FramedViewItem::updatePauseSprite() {}
void cgss::FramedViewItem::updateCursorSprite() {}
void cgss::FramedViewItem::resetPausePosition() {}
void cgss::FramedViewItem::updateBackOpacity() {}
void cgss::FramedViewItem::updateVerticesBlt(long, long) {}
void cgss::FramedViewItem::updateVerticesStretch(long, long) {}
void cgss::FramedViewItem::allocateVerticesBlt(long, long, long, long) {}
void cgss::FramedViewItem::allocateVerticesStretch() {}

void cgss::FramedViewItem::updateContents() {
	setCursorRectangle(m_cursorRect);
}

void cgss::FramedViewItem::updateContentsOpacity() {
	const auto opacity = static_cast<uint8_t>((m_opacity * m_contentOpacity) / 255.f);
	drawables().updateContentsOpacity(opacity);
}

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

void cgss::FramedViewItem::setPauseSkin(cgss::Texture& texture) {
	m_impl->pauseSkin = &texture;
}

void cgss::FramedViewItem::pause(bool pause) {
	m_paused = pause;
}
bool cgss::FramedViewItem::isPaused() const { return m_paused; }

void cgss::FramedViewItem::setPausePosition(float x, float y) {
	m_pauseX = static_cast<long>(x);
	m_pauseY = static_cast<long>(y);
}

void cgss::FramedViewItem::setCursorSkin(cgss::Texture& texture) {
	m_impl->cursorSkin = &texture;
}

void cgss::FramedViewItem::active(bool active) { m_active = active; }
bool cgss::FramedViewItem::isActive() const { return m_active; }

void cgss::FramedViewItem::onMove() {
	updateVertices();
	updateView();
}
void cgss::FramedViewItem::onResize() {
	updateVertices();
	updateView();
}

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

void cgss::FramedViewItem::move(float x, float y) {
	m_x = x;
	m_y = y;
	updateView();
}

void cgss::FramedViewItem::updateFromValue(const cgss::IntRect* rectangle) {
	setCursorRectangle(rectangle == nullptr ? cgss::IntRect{} : *rectangle);
}

// ---- FramedView (proxy forwarder; backend-agnostic) ----

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
