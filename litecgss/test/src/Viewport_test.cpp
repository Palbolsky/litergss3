#include <iostream>
#include <doctest.h>

#include "Views/Viewport.h"
#include "Graphics/Sprite.h"
#include "Views/DisplayWindow.h"

struct ViewportDataTest {
	cgss::DisplayWindowSettings settings{};
	cgss::MainEventDispatcher eventDispatcher{};
	cgss::DisplayWindow window{ settings };
};

static inline ViewportDataTest& PrepareTest() {
	static auto data = ViewportDataTest{};
	data.window.clearViews();
	return data;
}

static inline auto& CreateViewport(ViewportDataTest& data = PrepareTest()) {
	return data.window.addView<cgss::Viewport>();
}

TEST_CASE("[Viewport] DrawableStack") {
	auto& data = PrepareTest();
	auto& stack = CreateViewport(data);
	auto& stack2 = CreateViewport(data);

	{
		auto& sprite = cgss::Sprite::create(stack);
		auto& sprite2 = cgss::Sprite::create(stack);

		CHECK(stack.getStackSize() == 2);

		sprite2.detach();

		CHECK(stack.getStackSize() == 1);

		sprite2.setVisible(false);

		auto& sprite3 = cgss::Sprite::create(stack2);

		CHECK(stack2.getStackSize() == 1);
	}

	CHECK(stack.getStackSize() == 1);
	CHECK(stack2.getStackSize() == 1);

}

TEST_CASE("[Viewport] DrawableStack width drawing") {
	auto& stack = CreateViewport();

	auto renderTarget = sf::RenderWindow{};

	auto& sprite2 = cgss::Sprite::create(stack);

	sprite2.detach();

	sprite2.setVisible(false);

	stack.draw(renderTarget);
	//Here sprite2 can not be used anymore (logical delete)
	//TODO check
}
