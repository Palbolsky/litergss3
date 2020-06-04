#include <doctest.h>

#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Graphics/Shape.h"

struct ShapeDataTest {
	cgss::DisplayWindowSettings settings{ true };
	cgss::MainEventDispatcher eventDispatcher{};
	cgss::DisplayWindow window { eventDispatcher, settings};
    cgss::Viewport viewport {cgss::Viewport::create(window, eventDispatcher, settings) };
};

static inline ShapeDataTest& PrepareTest() {
	static auto data = ShapeDataTest{};
	return data;
}

static inline cgss::Shape CreateRectangleShape(ShapeDataTest& data = PrepareTest()) {
	auto innerShape = std::make_unique<sf::RectangleShape>();
    auto innerType = cgss::ShapeType::Rectangle;
    auto instance = cgss::ShapeData{ innerType, std::move(innerShape) };
    return cgss::Shape::create(data.viewport, std::move(instance));
}

TEST_CASE("[Shape] Rectangle Creation") {
	CreateRectangleShape();
}
