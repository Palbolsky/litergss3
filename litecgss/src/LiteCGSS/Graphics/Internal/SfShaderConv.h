#ifndef CGSS_INTERNAL_SF_SHADER_CONV_H
#define CGSS_INTERNAL_SF_SHADER_CONV_H

// Internal header: included only by implementation .cpp files that need to
// bridge cgss shader/blend types with their sf:: counterparts at the backend
// boundary. Not part of the public API — consumers of LiteCGSS must never
// include this.

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Shader.hpp>

#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"

namespace cgss::internal {

	// Blend enum conversions — numeric values match by construction.
	inline sf::BlendMode::Factor toSf(cgss::BlendFactor f) noexcept {
		return static_cast<sf::BlendMode::Factor>(f);
	}
	inline sf::BlendMode::Equation toSf(cgss::BlendEquation e) noexcept {
		return static_cast<sf::BlendMode::Equation>(e);
	}
	inline cgss::BlendFactor fromSf(sf::BlendMode::Factor f) noexcept {
		return static_cast<cgss::BlendFactor>(f);
	}
	inline cgss::BlendEquation fromSf(sf::BlendMode::Equation e) noexcept {
		return static_cast<cgss::BlendEquation>(e);
	}

	// Shader stage enum.
	inline sf::Shader::Type toSf(cgss::ShaderType t) noexcept {
		return static_cast<sf::Shader::Type>(t);
	}

	// GLSL uniform value conversions.
	inline sf::Glsl::Vec2 toSf(const cgss::Glsl::Vec2& v) noexcept {
		return sf::Glsl::Vec2(v.x, v.y);
	}
	inline sf::Glsl::Vec3 toSf(const cgss::Glsl::Vec3& v) noexcept {
		return sf::Glsl::Vec3(v.x, v.y, v.z);
	}
	inline sf::Glsl::Vec4 toSf(const cgss::Glsl::Vec4& v) noexcept {
		return sf::Glsl::Vec4(v.x, v.y, v.z, v.w);
	}

	inline sf::Glsl::Ivec2 toSf(const cgss::Glsl::Ivec2& v) noexcept {
		return sf::Glsl::Ivec2(v.x, v.y);
	}
	inline sf::Glsl::Ivec3 toSf(const cgss::Glsl::Ivec3& v) noexcept {
		return sf::Glsl::Ivec3(v.x, v.y, v.z);
	}
	inline sf::Glsl::Ivec4 toSf(const cgss::Glsl::Ivec4& v) noexcept {
		return sf::Glsl::Ivec4(v.x, v.y, v.z, v.w);
	}

	inline sf::Glsl::Bvec2 toSf(const cgss::Glsl::Bvec2& v) noexcept {
		return sf::Glsl::Bvec2(v.x, v.y);
	}
	inline sf::Glsl::Bvec3 toSf(const cgss::Glsl::Bvec3& v) noexcept {
		return sf::Glsl::Bvec3(v.x, v.y, v.z);
	}
	inline sf::Glsl::Bvec4 toSf(const cgss::Glsl::Bvec4& v) noexcept {
		return sf::Glsl::Bvec4(v.x, v.y, v.z, v.w);
	}

	inline sf::Glsl::Mat3 toSf(const cgss::Glsl::Mat3& m) noexcept {
		return sf::Glsl::Mat3(m.array);
	}
	inline sf::Glsl::Mat4 toSf(const cgss::Glsl::Mat4& m) noexcept {
		return sf::Glsl::Mat4(m.array);
	}

}

#endif
