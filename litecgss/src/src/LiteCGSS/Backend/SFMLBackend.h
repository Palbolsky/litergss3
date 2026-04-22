#ifndef CGSS_BACKEND_SFML_BACKEND_H
#define CGSS_BACKEND_SFML_BACKEND_H

// Full SFML 2.6 reference backend: includes SFMLBackendFwd.h (typedefs +
// capability tags) and defines `SFMLBackend::Ops` with all inline static
// thunks. Header-only by design — every operation is `static inline` so
// release builds inline the wrapper down to the direct SFML call.
//
// Include this only from implementation .cpp files. Public templates use
// SFMLBackendFwd.h to avoid pulling SFML into consumer TUs.

#include "LiteCGSS/Backend/SFMLBackendFwd.h"

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/Transformable.h"

namespace cgss::backend {

	// Definition of the nested Ops struct forward-declared in SFMLBackendFwd.h.
	// Each method is a thin `static inline` thunk over an SFML call so the
	// compiler can inline it through the wrapper template at the use site.
	struct SFMLBackend::Ops {

		// ---- Value conversions ----
		// IntRect
		static inline sf::IntRect toNative(const cgss::IntRect& r) noexcept {
			return sf::IntRect(r.left, r.top, r.width, r.height);
		}
		static inline cgss::IntRect fromNative(const sf::IntRect& r) noexcept {
			return cgss::IntRect{ r.left, r.top, r.width, r.height };
		}

		// Color
		static inline sf::Color toNative(const cgss::Color& c) noexcept {
			return sf::Color(c.r, c.g, c.b, c.a);
		}
		static inline cgss::Color fromNative(const sf::Color& c) noexcept {
			return cgss::Color{ c.r, c.g, c.b, c.a };
		}

		// Blend enums (numeric values match by construction)
		static inline sf::BlendMode::Factor   toNative(cgss::BlendFactor f)    noexcept { return static_cast<sf::BlendMode::Factor>(f); }
		static inline sf::BlendMode::Equation toNative(cgss::BlendEquation e)  noexcept { return static_cast<sf::BlendMode::Equation>(e); }
		static inline cgss::BlendFactor       fromNative(sf::BlendMode::Factor f)   noexcept { return static_cast<cgss::BlendFactor>(f); }
		static inline cgss::BlendEquation     fromNative(sf::BlendMode::Equation e) noexcept { return static_cast<cgss::BlendEquation>(e); }

		// Shader stage
		static inline sf::Shader::Type toNative(cgss::ShaderType t) noexcept {
			return static_cast<sf::Shader::Type>(t);
		}

		// GLSL uniform values
		static inline sf::Glsl::Vec2 toNative(const cgss::Glsl::Vec2& v) noexcept { return sf::Glsl::Vec2(v.x, v.y); }
		static inline sf::Glsl::Vec3 toNative(const cgss::Glsl::Vec3& v) noexcept { return sf::Glsl::Vec3(v.x, v.y, v.z); }
		static inline sf::Glsl::Vec4 toNative(const cgss::Glsl::Vec4& v) noexcept { return sf::Glsl::Vec4(v.x, v.y, v.z, v.w); }

		static inline sf::Glsl::Ivec2 toNative(const cgss::Glsl::Ivec2& v) noexcept { return sf::Glsl::Ivec2(v.x, v.y); }
		static inline sf::Glsl::Ivec3 toNative(const cgss::Glsl::Ivec3& v) noexcept { return sf::Glsl::Ivec3(v.x, v.y, v.z); }
		static inline sf::Glsl::Ivec4 toNative(const cgss::Glsl::Ivec4& v) noexcept { return sf::Glsl::Ivec4(v.x, v.y, v.z, v.w); }

		static inline sf::Glsl::Bvec2 toNative(const cgss::Glsl::Bvec2& v) noexcept { return sf::Glsl::Bvec2(v.x, v.y); }
		static inline sf::Glsl::Bvec3 toNative(const cgss::Glsl::Bvec3& v) noexcept { return sf::Glsl::Bvec3(v.x, v.y, v.z); }
		static inline sf::Glsl::Bvec4 toNative(const cgss::Glsl::Bvec4& v) noexcept { return sf::Glsl::Bvec4(v.x, v.y, v.z, v.w); }

		static inline sf::Glsl::Mat3 toNative(const cgss::Glsl::Mat3& m) noexcept { return sf::Glsl::Mat3(m.array); }
		static inline sf::Glsl::Mat4 toNative(const cgss::Glsl::Mat4& m) noexcept { return sf::Glsl::Mat4(m.array); }

		// Context settings
		static inline sf::ContextSettings toNative(const cgss::DisplayWindowContextSettings& c) noexcept {
			return sf::ContextSettings(
				c.depthBits,
				c.stencilBits,
				c.antialiasingLevel,
				c.majorVersion,
				c.minorVersion,
				c.attributeFlags,
				c.sRgbCapable
			);
		}
		static inline cgss::DisplayWindowContextSettings fromNative(const sf::ContextSettings& c) noexcept {
			return cgss::DisplayWindowContextSettings{
				c.depthBits,
				c.stencilBits,
				c.antialiasingLevel,
				c.majorVersion,
				c.minorVersion,
				c.attributeFlags,
				c.sRgbCapable
			};
		}

		// ---- Transform composition ----
		static inline sf::Transform build_transform(const cgss::Transformable& xf) noexcept {
			sf::Transform t;
			t.translate(xf.getX(), xf.getY());
			t.rotate(-xf.getAngle());
			t.scale(xf.getScaleX(), xf.getScaleY());
			t.translate(-xf.getOx(), -xf.getOy());
			return t;
		}

		// ---- Draw thunks ----
		static inline void draw_sprite(native_render_target& target,
		                               const native_sprite& sprite,
		                               const native_render_states& states) {
			target.draw(sprite, states);
		}

		// ---- Texture / Image ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			t.setSmooth(smooth);
		}

		static inline Size2u get_texture_size(const native_texture& t) noexcept {
			const auto sz = t.getSize();
			return { sz.x, sz.y };
		}

		static inline Size2u get_image_size(const native_image& i) noexcept {
			const auto sz = i.getSize();
			return { sz.x, sz.y };
		}

		static inline bool create_texture(native_texture& t, unsigned int width, unsigned int height) {
			return t.create(width, height);
		}

		static inline void update_texture_from_image(native_texture& t, const native_image& i) {
			t.update(i);
		}

		static inline void copy_image(native_image& dst, const native_image& src) {
			dst = src;
		}

		static inline void image_set_pixel(native_image& i, unsigned int x, unsigned int y, cgss::Color c) {
			i.setPixel(x, y, sf::Color(c.r, c.g, c.b, c.a));
		}

		static inline void image_blit(native_image& dst, unsigned int x, unsigned int y,
		                              const native_image& src, const cgss::IntRect& srcRect) {
			dst.copy(src, x, y, sf::IntRect(srcRect.left, srcRect.top, srcRect.width, srcRect.height));
		}
	};

}

#endif
