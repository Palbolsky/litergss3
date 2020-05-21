#include <iostream>
#include <doctest.h>

#include "Views/Viewport.h"
#include "Graphics/Sprite.h"
#include "Views/DisplayWindow.h"

struct ViewportDataTest {
	cgss::DisplayWindowSettings settings{ true };
	cgss::MainEventDispatcher eventDispatcher{};
	cgss::DisplayWindow window{ eventDispatcher, settings };
};

static inline ViewportDataTest& PrepareTest() {
	static auto data = ViewportDataTest{};
	return data;
}

static inline auto CreateViewport(ViewportDataTest& data = PrepareTest()) {
	return data.window.addView<cgss::Viewport>();
}


TEST_CASE("[DrawableStack] DrawableStack : drawable dies before") {
	auto& data = PrepareTest();
	auto& stack = CreateViewport(data);

	{
		auto sprite = cgss::Sprite::create(stack);
		CHECK(stack.getStackSize() == 1);
	}

	CHECK(stack.getStackSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : stack dies before (moved Drawable)") {
	auto& data = PrepareTest();

	auto sprite2 = cgss::Sprite{};
	{
		auto stack = CreateViewport(data);
		auto sprite = cgss::Sprite::create(stack);
		sprite2 = std::move(sprite);
		CHECK(stack.getStackSize() == 1);
	}

	// Still usable safely, but no more presence on screen
	sprite2.move(0.f, 12.f);
}

TEST_CASE("[DrawableStack] DrawableStack : drawable is disposed explicitely") {
	auto& data = PrepareTest();
	auto& stack = CreateViewport(data);

	{
		auto sprite = cgss::Sprite::create(stack);
		CHECK(stack.getStackSize() == 1);
		sprite.dispose();
		CHECK(stack.getStackSize() == 0);
	}

	CHECK(stack.getStackSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : ensure moved sprite is disposed") {
	auto& data = PrepareTest();
	auto stack = CreateViewport(data);
	
	auto sprite = cgss::Sprite::create(stack);
	auto sprite2 = cgss::Sprite::create(stack);
	CHECK(stack.getStackSize() == 2);
	sprite = std::move(sprite2);
	CHECK(stack.getStackSize() == 1);
}

TEST_CASE("[DrawableStack] DrawableStack : dispose several times") {
	auto& data = PrepareTest();
	auto stack = CreateViewport(data);

	auto sprite = cgss::Sprite::create(stack);
	CHECK(stack.getStackSize() == 1);
	sprite.dispose();
	CHECK(stack.getStackSize() == 0);
	sprite.dispose();
	CHECK(stack.getStackSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : complex mix") {
	auto& data = PrepareTest();
	auto stack = CreateViewport(data);
	auto stack2 = CreateViewport(data);

	{
		auto sprite = cgss::Sprite::create(stack);
		auto sprite2 = cgss::Sprite::create(stack);

		CHECK(stack.getStackSize() == 2);

		sprite2.dispose();

		CHECK(stack.getStackSize() == 1);

		auto sprite3 = cgss::Sprite::create(stack2);

		CHECK(stack2.getStackSize() == 1);

		sprite = std::move(sprite3);

		CHECK(stack.getStackSize() == 0);
		CHECK(stack2.getStackSize() == 1);
	}

	CHECK(stack.getStackSize() == 0);
	CHECK(stack2.getStackSize() == 0);

}

TEST_CASE("[DrawableStack] DrawableStack width drawing") {
	auto stack = CreateViewport();

	auto renderTarget = sf::RenderWindow{};

	auto sprite2 = cgss::Sprite::create(stack);
	sprite2.setAngle(5.f);

	// Here sprite2 is deleted from stack
	sprite2.dispose();
	
	CHECK(stack.getStackSize() == 0);
	
	// We still can use the object safely, it just has no effects anymore
	stack.draw(renderTarget);

	const auto oldAngle = static_cast<int>(sprite2.getAngle());
	CHECK(oldAngle == -355);
	sprite2.setAngle(123.0);
	
}
