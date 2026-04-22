#ifndef CGSS_INT_RECT_H
#define CGSS_INT_RECT_H

namespace cgss {

	// Backend-agnostic integer rectangle. Structurally equivalent to sf::IntRect
	// (same field names, same types, same order) so that backend adapters can
	// convert cheaply at .cpp boundaries — see Common/Internal/SfRectConv.h.
	struct IntRect {
		int left = 0;
		int top = 0;
		int width = 0;
		int height = 0;

		IntRect() = default;
		IntRect(int l, int t, int w, int h) noexcept : left(l), top(t), width(w), height(h) {}

		bool operator==(const IntRect& other) const noexcept {
			return left == other.left && top == other.top
				&& width == other.width && height == other.height;
		}
		bool operator!=(const IntRect& other) const noexcept { return !(*this == other); }
	};

}

#endif
