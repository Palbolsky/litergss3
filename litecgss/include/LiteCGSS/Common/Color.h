#ifndef CGSS_COLOR_H
#define CGSS_COLOR_H

#include <cstdint>

namespace cgss {

	// Backend-agnostic RGBA color (8-bit unsigned channels). Backends convert
	// it to their native color type at the boundary.
	struct Color {
		std::uint8_t r = 0;
		std::uint8_t g = 0;
		std::uint8_t b = 0;
		std::uint8_t a = 255;

		constexpr bool operator==(const Color& other) const noexcept {
			return r == other.r && g == other.g && b == other.b && a == other.a;
		}
		constexpr bool operator!=(const Color& other) const noexcept { return !(*this == other); }
	};

	namespace Colors {
		inline constexpr Color Black       { 0,   0,   0,   255 };
		inline constexpr Color White       { 255, 255, 255, 255 };
		inline constexpr Color Red         { 255, 0,   0,   255 };
		inline constexpr Color Green       { 0,   255, 0,   255 };
		inline constexpr Color Blue        { 0,   0,   255, 255 };
		inline constexpr Color Yellow      { 255, 255, 0,   255 };
		inline constexpr Color Magenta     { 255, 0,   255, 255 };
		inline constexpr Color Cyan        { 0,   255, 255, 255 };
		inline constexpr Color Transparent { 0,   0,   0,   0   };
	}

}

#endif
