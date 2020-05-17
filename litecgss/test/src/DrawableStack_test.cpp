#include <doctest.h>

#include "Views/Stack/DrawableStack.h"
#include "Graphics/Drawable.h"

struct DrawableTest;
struct DrawableStackItemTest : 
	public cgss::GraphicsStackItem {
	DrawableStackItemTest(DrawableTest*) {}
protected:
	void draw(sf::RenderTarget& target) const override {};
	void drawFast(sf::RenderTarget& target) const override {};
};

struct DrawableTest : 
	public cgss::Drawable<DrawableTest, DrawableStackItemTest> {

	bool hasProxy() const { return cgss::Drawable<DrawableTest, DrawableStackItemTest>::hasProxy(); }
};


TEST_CASE("[DrawableStack] DrawableStack : drawable dies before") {
	auto stack = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		CHECK(stack.getSize() == 1);
	}

	CHECK(stack.getSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : stack dies before (moved Drawable)") {
	auto sprite2 = DrawableTest{};
	{
		auto stack = cgss::DrawableStack{};
		auto sprite = DrawableTest::create(stack);
		sprite2 = std::move(sprite);
		CHECK(stack.getSize() == 1);
	}

	// Still usable safely, but no more presence on screen
	CHECK(sprite2.hasProxy());
}

TEST_CASE("[DrawableStack] DrawableStack : drawable is disposed explicitely") {
	auto& stack = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		CHECK(stack.getSize() == 1);
		sprite.dispose();
		CHECK(stack.getSize() == 0);
	}

	CHECK(stack.getSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : ensure moved sprite is disposed") {
	auto stack = cgss::DrawableStack{};
	
	auto sprite = DrawableTest::create(stack);
	auto sprite2 = DrawableTest::create(stack);
	CHECK(stack.getSize() == 2);
	sprite = std::move(sprite2);
	CHECK(stack.getSize() == 1);
}

TEST_CASE("[DrawableStack] DrawableStack : dispose several times") {
	auto stack = cgss::DrawableStack{};

	auto sprite = DrawableTest::create(stack);
	CHECK(stack.getSize() == 1);
	sprite.dispose();
	CHECK(stack.getSize() == 0);
	sprite.dispose();
	CHECK(stack.getSize() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : complex mix") {
	auto stack = cgss::DrawableStack{};
	auto stack2 = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		auto sprite2 = DrawableTest::create(stack);

		CHECK(stack.getSize() == 2);

		sprite2.dispose();

		CHECK(stack.getSize() == 1);

		auto sprite3 = DrawableTest::create(stack2);

		CHECK(stack2.getSize() == 1);

		sprite = std::move(sprite3);

		CHECK(stack.getSize() == 0);
		CHECK(stack2.getSize() == 1);
	}

	CHECK(stack.getSize() == 0);
	CHECK(stack2.getSize() == 0);

}

TEST_CASE("[DrawableStack] DrawableStack width drawing") {
	auto stack = cgss::DrawableStack{};

	auto renderTarget = sf::RenderWindow{};

	auto sprite2 = DrawableTest::create(stack);
	CHECK(sprite2.hasProxy());

	// Here sprite2 is deleted from stack
	sprite2.dispose();
	
	CHECK(stack.getSize() == 0);
	
	// We still can use the object safely, it just has no effects anymore
	auto view = renderTarget.getView();
	stack.draw(view, renderTarget);

	CHECK(sprite2.hasProxy());
}
