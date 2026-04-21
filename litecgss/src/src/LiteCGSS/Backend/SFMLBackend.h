#ifndef CGSS_BACKEND_SFML_BACKEND_H
#define CGSS_BACKEND_SFML_BACKEND_H

// SFML 2.6 reference backend. Header-only by design: every operation is a
// `static inline` thunk over the underlying SFML call, so release builds
// inline the wrapper down to the direct call.
//
// The typedef and thunk surface grows as wrappers are templated on
// `Backend` (see plan PR C onward). PR A seeds only: native typedefs,
// capability tags, the value-type conversions formerly in
// Common/Internal/Sf*Conv.h + Graphics/Internal/SfShaderConv.h +
// Configuration/Internal/SfContextSettingsConv.h, and one proof-of-concept
// draw thunk.

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

#include <SFML/Graphics/Transform.hpp>

namespace cgss::backend {

	struct SFMLBackend {
		// ---- Native typedefs ----
		using native_render_target  = sf::RenderTarget;
		using native_texture        = sf::Texture;
		using native_image          = sf::Image;
		using native_sprite         = sf::Sprite;
		using native_view           = sf::View;
		using native_render_states  = sf::RenderStates;
		using native_font           = sf::Font;
		using native_event          = sf::Event;
		using native_window         = sf::RenderWindow;
		using native_render_texture = sf::RenderTexture;
		using native_shader         = sf::Shader;
		using native_vertex_array   = sf::VertexArray;

		// ---- Capability tags ----
		static constexpr bool has_window      = true;
		static constexpr bool has_event_pump  = true;
		static constexpr bool owns_gpu_memory = true;

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
		// Builds an sf::Transform equivalent to the transform that sf::Transformable
		// applies internally, from the cgss::Transformable's own state. Sign of the
		// angle is flipped to match SFML's rotation convention (clockwise when Y
		// points down, as with screen coordinates).
		static inline sf::Transform build_transform(const cgss::Transformable& xf) noexcept {
			sf::Transform t;
			t.translate(xf.getX(), xf.getY());
			t.rotate(-xf.getAngle());
			t.scale(xf.getScaleX(), xf.getScaleY());
			t.translate(-xf.getOx(), -xf.getOy());
			return t;
		}

		// ---- Draw thunks (seed; grown per wrapper PR) ----
		static inline void draw_sprite(native_render_target& target,
		                               const native_sprite& sprite,
		                               const native_render_states& states) {
			target.draw(sprite, states);
		}
	};

}

#endif
