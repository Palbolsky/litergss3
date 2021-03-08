#include "Rectangle.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"

void cgss::Rectangle::setX(long x) {
	edit([&x](sf::IntRect& rect) {
		rect.left = x;
	});
}

void cgss::Rectangle::setY(long y) {
	edit([&y](sf::IntRect& rect) {
		rect.top = y;
	});
}

void cgss::Rectangle::setWidth(long width) {
	edit([&width](sf::IntRect& rect) {
		rect.width = width;
	});
}

void cgss::Rectangle::setHeight(long height) {
	edit([&height](sf::IntRect& rect) {
		rect.height = height;
	});
}
