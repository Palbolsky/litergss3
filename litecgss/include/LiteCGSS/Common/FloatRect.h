#ifndef CGSS_FLOAT_RECT_H
#define CGSS_FLOAT_RECT_H

namespace cgss {

	// Backend-agnostic float rectangle. Structurally equivalent to sf::FloatRect
	// (same field names, same types, same order) so that SFML adapters can
	// memcpy / aggregate-init at .cpp boundaries — see Backend/Internal/
	// SFMLConversions.h. Mirrors the cgss::IntRect shape.
	struct FloatRect {
		float left = 0.f;
		float top = 0.f;
		float width = 0.f;
		float height = 0.f;

		FloatRect() = default;
		FloatRect(float l, float t, float w, float h) noexcept : left(l), top(t), width(w), height(h) {}

		bool operator==(const FloatRect& other) const noexcept {
			return left == other.left && top == other.top
				&& width == other.width && height == other.height;
		}
		bool operator!=(const FloatRect& other) const noexcept { return !(*this == other); }
	};

}

#endif
