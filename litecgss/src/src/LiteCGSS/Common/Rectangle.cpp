#include "Rectangle.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"

void cgss::Rectangle::setX(long x) {
	edit([&x](cgss::IntRect& rect) {
		rect.left = x;
	});
}

void cgss::Rectangle::setY(long y) {
	edit([&y](cgss::IntRect& rect) {
		rect.top = y;
	});
}

void cgss::Rectangle::setWidth(long width) {
	edit([&width](cgss::IntRect& rect) {
		rect.width = width;
	});
}

void cgss::Rectangle::setHeight(long height) {
	edit([&height](cgss::IntRect& rect) {
		rect.height = height;
	});
}
