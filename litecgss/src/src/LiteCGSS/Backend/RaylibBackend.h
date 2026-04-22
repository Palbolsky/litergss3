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

		// ---- Draw thunks (stubs) ----
		static inline void draw_sprite(native_render_target&, const native_sprite&, const native_render_states&) {}

		// ---- Sprite ops (stubs) ----
		static inline cgss::Color get_sprite_color(const native_sprite&) noexcept { return cgss::Colors::White; }
		static inline void set_sprite_color(native_sprite&, cgss::Color) noexcept {}
		static inline cgss::IntRect get_sprite_texture_rect(const native_sprite&) noexcept { return {}; }
		static inline void set_sprite_texture_rect(native_sprite&, const cgss::IntRect&) noexcept {}
		static inline void set_sprite_texture(native_sprite&, const native_texture&, bool) noexcept {}
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
