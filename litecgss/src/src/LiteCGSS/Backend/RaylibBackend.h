#ifndef CGSS_BACKEND_RAYLIB_BACKEND_H
#define CGSS_BACKEND_RAYLIB_BACKEND_H

// Raylib backend — scaffold only (PR M scope b).
//
// Defines `RaylibBackend::Ops` so that wrappers whose bodies route exclusively
// through `B::Ops::*` (Camera, DrawState, Font, Event, RenderTarget) can
// instantiate under `CGSS_BACKEND=raylib`. Texture / Image / Sprite / Shape /
// DisplayWindow currently FAIL to instantiate — by design, to expose where
// further work is required:
//
//   * Texture/Image: `create(filename)` calls `TextureFileSerializeMethod` /
//     `ImageFileSerializeMethod`, both SFML-typed (PR K deferred templating
//     of the SerializeMethod structs themselves).
//   * Sprite/Shape: bodies build `sf::RenderStates` and call `.draw(sprite,
//     states)` on the native render target — SFML-shaped APIs with no
//     drop-in raylib equivalent.
//   * DisplayWindow: body is fully SFML-coupled (PR J); needs a full
//     `basic_DisplayWindow<RaylibBackend>` specialization.
//
// Op bodies below are intentionally stubs (no raylib lib linked yet). The
// goal is to validate the abstraction's *shape* — that backend selection
// flips through CMake without changing public-API names — not to render
// anything.

#include "LiteCGSS/Backend/RaylibBackendFwd.h"

#include <cstdint>
#include <cstddef>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Transformable.h"

namespace cgss::backend {

	struct RaylibBackend::Ops {
		// ---- Wrapper native accessors (same shape as SFMLBackend::Ops) ----
		template <class W>
		static inline auto& native_of(W& w) noexcept { return w.native(); }
		template <class W>
		static inline const auto& native_of(const W& w) noexcept { return w.native(); }

		// ---- Transform composition ----
		// Placeholder — raylib has no sf::Transform analogue (its draw functions
		// take position/rotation/scale as separate args). Returns a small POD
		// to satisfy the call-site type system.
		struct Transform2D {
			float x = 0.f, y = 0.f;
			float ox = 0.f, oy = 0.f;
			float angle = 0.f;
			float scaleX = 1.f, scaleY = 1.f;
		};
		static inline Transform2D build_transform(const cgss::Transformable& xf) noexcept {
			return Transform2D{ xf.getX(), xf.getY(), xf.getOx(), xf.getOy(), xf.getAngle(), xf.getScaleX(), xf.getScaleY() };
		}

		// ---- Window ops (stubs) ----
		// raylib uses GLOBAL window state (InitWindow / CloseWindow / etc.) — the
		// `raylib::Window` placeholder is empty. A real impl would call
		// raylib's free functions and ignore the placeholder arg.
		static inline void window_close(native_window&) {}
		static inline bool window_is_open(const native_window&) { return false; }
		static inline void window_clear(native_window&) {}
		static inline void window_display(native_window&) {}
		static inline void window_set_active(native_window&, bool) {}
		static inline void window_set_title(native_window&, const char*) {}
		static inline void window_set_size(native_window&, unsigned int, unsigned int) {}
		static inline Size2u window_get_size(const native_window&) noexcept { return {}; }
		static inline void window_set_mouse_cursor_visible(native_window&, bool) {}
		static inline void window_set_vsync(native_window&, bool) {}
		static inline void window_set_framerate_limit(native_window&, unsigned int) {}
		static inline void window_set_position(native_window&, int, int) {}
		static inline Point2i window_get_position(const native_window&) noexcept { return {}; }
		static inline native_view window_get_default_view(const native_window&) { return {}; }
		static inline void window_set_view(native_window&, const native_view&) {}
		static inline cgss::DisplayWindowContextSettings window_get_settings(const native_window&) {
			return {};
		}
		static inline bool window_poll_event(native_window&, native_event&) { return false; }
		static inline bool window_wait_event(native_window&, native_event&) { return false; }
		static inline Size2u desktop_size() noexcept { return {}; }
		static inline void window_set_icon(native_window&, unsigned int, unsigned int, const std::uint8_t*) {}
		static inline void window_draw_render_texture(native_window&, const native_render_texture&, const native_render_states*) {}
		static inline void window_draw_color_quad(native_window&, float, float, float, float, cgss::Color) {}

		// ---- Render texture ops (stubs) ----
		static inline bool render_texture_create(native_render_texture&, unsigned int, unsigned int) { return false; }
		static inline void render_texture_set_smooth(native_render_texture&, bool) {}
		static inline const native_texture& render_texture_get_texture(const native_render_texture& rt) { return rt.texture; }
		static inline void render_texture_set_view(native_render_texture&, const native_view&) {}
		static inline void render_texture_display(native_render_texture&) {}
		static inline void render_texture_clear(native_render_texture&) {}
		static inline void render_texture_clear_color(native_render_texture&, cgss::Color) {}

		// ---- View ops (stubs) ----
		// raylib's Camera2D models offset/target/rotation/zoom (transform-
		// based), unlike sf::View's rectangular world-space region. A
		// faithful mapping needs auxiliary state per cgss::Camera; for now
		// these stubs no-op so the abstraction compiles.
		static inline Size2f view_get_size(const native_view&) noexcept { return {}; }
		static inline void view_set_size(native_view&, float, float) noexcept {}
		static inline Size2f view_get_center(const native_view& v) noexcept { return { v.target.x, v.target.y }; }
		static inline void view_set_center(native_view& v, float x, float y) noexcept { v.target = { x, y }; }
		static inline Box2f view_get_viewport(const native_view&) noexcept { return {}; }
		static inline void view_set_viewport(native_view&, float, float, float, float) noexcept {}
		static inline void view_reset(native_view&, float, float, float, float) noexcept {}
		static inline void view_zoom(native_view& v, float factor) noexcept { v.zoom *= factor; }
		static inline void view_set_rotation(native_view& v, float angle) noexcept { v.rotation = angle; }

		// ---- Draw-state ops (stubs) ----
		static inline void draw_state_set_shader(native_render_states&, native_shader*) noexcept {}
		static inline void draw_state_set_blend_factor(native_render_states&, BlendFactorSlot, cgss::BlendFactor) noexcept {}
		static inline cgss::BlendFactor draw_state_get_blend_factor(const native_render_states&, BlendFactorSlot) noexcept { return cgss::BlendFactor::Zero; }
		static inline void draw_state_set_blend_equation(native_render_states&, BlendEquationSlot, cgss::BlendEquation) noexcept {}
		static inline cgss::BlendEquation draw_state_get_blend_equation(const native_render_states&, BlendEquationSlot) noexcept { return cgss::BlendEquation::Add; }

		// ---- Render-target view ops (stubs) ----
		static inline void target_set_view(native_render_target&, const native_view&) {}
		static inline native_view target_get_view(const native_render_target&) { return {}; }
		static inline native_view target_get_default_view(const native_render_target&) { return {}; }

		// ---- Draw thunks (stubs) ----
		static inline void draw_sprite(native_render_target&, const native_sprite&, const native_render_states&) {}
		static inline void draw_sprite_transformed(native_render_target&, const native_sprite&, const cgss::Transformable&, const native_render_states*) {}
		static inline void draw_shape_transformed(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const cgss::Transformable&, const native_render_states*) {}

		// ---- Sprite ops (stubs) ----
		static inline cgss::Color get_sprite_color(const native_sprite&) noexcept { return cgss::Colors::White; }
		static inline void set_sprite_color(native_sprite&, cgss::Color) noexcept {}
		static inline cgss::IntRect get_sprite_texture_rect(const native_sprite&) noexcept { return {}; }
		static inline void set_sprite_texture_rect(native_sprite&, const cgss::IntRect&) noexcept {}
		static inline void set_sprite_texture(native_sprite&, const native_texture&, bool) noexcept {}
		static inline void sprite_set_position(native_sprite&, float, float) noexcept {}
		static inline Size2u get_sprite_bound_texture_size(const native_sprite&) noexcept { return { 0u, 0u }; }

		// ---- Shape ops (stubs) ----
		static inline void draw_shape(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const native_render_states&) {}
		static inline cgss::Point get_shape_point(const cgss::ShapeGeometry&, std::size_t) { return {}; }
		struct Bounds2f { float width = 0.f; float height = 0.f; };
		static inline Bounds2f get_shape_global_bounds(const cgss::ShapeGeometry&, const cgss::Transformable&) { return {}; }

		// ---- Texture ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			// real impl would call SetTextureFilter(t, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
			(void)t; (void)smooth;
		}

		static inline Size2u get_texture_size(const native_texture& t) noexcept {
			return { static_cast<unsigned int>(t.width), static_cast<unsigned int>(t.height) };
		}

		static inline bool create_texture(native_texture& t, unsigned int w, unsigned int h) {
			// real impl would build via LoadTextureFromImage(GenImageColor(w, h, BLANK))
			(void)t; (void)w; (void)h;
			return false;
		}

		static inline void update_texture_from_image(native_texture& t, const native_image& i) {
			// real impl: UpdateTexture(t, i.data);
			(void)t; (void)i;
		}

		// ---- Image ops ----
		static inline Size2u get_image_size(const native_image& i) noexcept {
			return { static_cast<unsigned int>(i.width), static_cast<unsigned int>(i.height) };
		}

		static inline void copy_image(native_image& dst, const native_image& src) {
			// raylib-real: dst = ImageCopy(src);
			// stub: shallow POD assign — NOT a deep copy of pixel data
			dst = src;
		}

		static inline void image_set_pixel(native_image& i, unsigned int x, unsigned int y, cgss::Color c) {
			// real: ImageDrawPixel(&i, x, y, {c.r, c.g, c.b, c.a});
			(void)i; (void)x; (void)y; (void)c;
		}

		static inline cgss::Color image_get_pixel(const native_image& i, unsigned int x, unsigned int y) {
			(void)i; (void)x; (void)y;
			return cgss::Colors::Black;
		}

		static inline void image_blit(native_image& dst, unsigned int x, unsigned int y,
		                              const native_image& src, const cgss::IntRect& srcRect,
		                              bool applyAlpha = false) {
			(void)dst; (void)x; (void)y; (void)src; (void)srcRect; (void)applyAlpha;
		}

		static inline const std::uint8_t* get_image_pixels_ptr(const native_image& i) noexcept {
			return reinterpret_cast<const std::uint8_t*>(i.data);
		}

		static inline void image_create_with_pixels(native_image& i, unsigned int w, unsigned int h, const std::uint8_t* pixels) {
			(void)i; (void)w; (void)h; (void)pixels;
		}

		static inline void image_create_mask_from_color(native_image& i, cgss::Color color, std::uint8_t alpha) {
			(void)i; (void)color; (void)alpha;
		}
	};

	// Free-function helper mirroring the SFMLBackend equivalent — lets impl
	// .cpps write `cgss::backend::native_of(w)` / `native_of(w)` (ADL).
	template <class W>
	inline auto& native_of(W& w) noexcept {
		return RaylibBackend::Ops::native_of(w);
	}
	template <class W>
	inline const auto& native_of(const W& w) noexcept {
		return RaylibBackend::Ops::native_of(w);
	}

}

#endif
