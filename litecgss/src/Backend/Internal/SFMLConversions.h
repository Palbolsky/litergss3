#ifndef CGSS_BACKEND_INTERNAL_SFML_CONVERSIONS_H
#define CGSS_BACKEND_INTERNAL_SFML_CONVERSIONS_H

// Backend-internal header: included by impl .cpp files that need to bridge
// cgss POD value types with their sf:: counterparts at the backend boundary.
// Not part of the public API — consumers of LiteCGSS must never include this.
//
// Consolidates the 4 former Internal/Sf*Conv.h headers under a single
// backend-neutral name set (toNative/fromNative, in cgss::backend::) so that
// ADL finds the right overload via either the cgss:: or sf:: argument type.
// The SFMLBackend::Ops struct uses these through `using cgss::backend::toNative;`
// declarations; other .cpp callers qualify with `cgss::backend::toNative(...)`.

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Window/ContextSettings.hpp>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"

namespace cgss::backend {

	// ---- IntRect ----
	inline sf::IntRect toNative(const cgss::IntRect& r) noexcept {
		return sf::IntRect(r.left, r.top, r.width, r.height);
	}
	inline cgss::IntRect fromNative(const sf::IntRect& r) noexcept {
		return cgss::IntRect{ r.left, r.top, r.width, r.height };
	}

	// ---- Color ----
	inline sf::Color toNative(const cgss::Color& c) noexcept {
		return sf::Color(c.r, c.g, c.b, c.a);
	}
	inline cgss::Color fromNative(const sf::Color& c) noexcept {
		return cgss::Color{ c.r, c.g, c.b, c.a };
	}

	// ---- Blend enums (numeric values match by construction) ----
	inline sf::BlendMode::Factor   toNative(cgss::BlendFactor f)      noexcept { return static_cast<sf::BlendMode::Factor>(f); }
	inline sf::BlendMode::Equation toNative(cgss::BlendEquation e)    noexcept { return static_cast<sf::BlendMode::Equation>(e); }
	inline cgss::BlendFactor       fromNative(sf::BlendMode::Factor f)   noexcept { return static_cast<cgss::BlendFactor>(f); }
	inline cgss::BlendEquation     fromNative(sf::BlendMode::Equation e) noexcept { return static_cast<cgss::BlendEquation>(e); }

	// ---- Shader stage ----
	inline sf::Shader::Type toNative(cgss::ShaderType t) noexcept {
		return static_cast<sf::Shader::Type>(t);
	}

	// ---- GLSL uniform values ----
	inline sf::Glsl::Vec2 toNative(const cgss::Glsl::Vec2& v) noexcept { return sf::Glsl::Vec2(v.x, v.y); }
	inline sf::Glsl::Vec3 toNative(const cgss::Glsl::Vec3& v) noexcept { return sf::Glsl::Vec3(v.x, v.y, v.z); }
	inline sf::Glsl::Vec4 toNative(const cgss::Glsl::Vec4& v) noexcept { return sf::Glsl::Vec4(v.x, v.y, v.z, v.w); }

	inline sf::Glsl::Ivec2 toNative(const cgss::Glsl::Ivec2& v) noexcept { return sf::Glsl::Ivec2(v.x, v.y); }
	inline sf::Glsl::Ivec3 toNative(const cgss::Glsl::Ivec3& v) noexcept { return sf::Glsl::Ivec3(v.x, v.y, v.z); }
	inline sf::Glsl::Ivec4 toNative(const cgss::Glsl::Ivec4& v) noexcept { return sf::Glsl::Ivec4(v.x, v.y, v.z, v.w); }

	inline sf::Glsl::Bvec2 toNative(const cgss::Glsl::Bvec2& v) noexcept { return sf::Glsl::Bvec2(v.x, v.y); }
	inline sf::Glsl::Bvec3 toNative(const cgss::Glsl::Bvec3& v) noexcept { return sf::Glsl::Bvec3(v.x, v.y, v.z); }
	inline sf::Glsl::Bvec4 toNative(const cgss::Glsl::Bvec4& v) noexcept { return sf::Glsl::Bvec4(v.x, v.y, v.z, v.w); }

	inline sf::Glsl::Mat3 toNative(const cgss::Glsl::Mat3& m) noexcept { return sf::Glsl::Mat3(m.array); }
	inline sf::Glsl::Mat4 toNative(const cgss::Glsl::Mat4& m) noexcept { return sf::Glsl::Mat4(m.array); }

	// ---- Context settings ----
	inline sf::ContextSettings toNative(const cgss::DisplayWindowContextSettings& c) noexcept {
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
	inline cgss::DisplayWindowContextSettings fromNative(const sf::ContextSettings& c) noexcept {
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

}

#endif
