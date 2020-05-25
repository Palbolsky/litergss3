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
	auto stack = cgss::DrawableStack{};

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

TEST_CASE("[DrawableStack] DrawableStack with drawing") {
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


TEST_CASE("[DrawableStack] Default z index") {
	auto stack = cgss::DrawableStack{};
	auto sprite = DrawableTest::create(stack);
	CHECK(sprite.getZ().z == std::numeric_limits<long>::max());
	CHECK(sprite.getZ().index == 0);
	
	auto sprite2 = DrawableTest::create(stack);
	CHECK(sprite2.getZ().z == std::numeric_limits<long>::max());
	CHECK(sprite2.getZ().index == 1);
}

TEST_CASE("[DrawableStack] z sorting") {
	constexpr const auto MaxStackNum = std::numeric_limits<long>::max();

	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<cgss::ZIndex> { {0, 0}, {0, 15}, {1, 1}, {1, 2}, {2, 33}, {3, 22}, {3, 55}, {4, 66}, {5, 55}, {6, 4}, {MaxStackNum, 5}, {MaxStackNum, 6}, {MaxStackNum, 7} };
	auto container = std::vector<DrawableTest>{};
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		auto drawable = DrawableTest::create(stack);
		drawable.setZ(zindexStack[index]);
		container.push_back(std::move(drawable));
		index++;
	}
	
	SUBCASE("Internal 'random' movements") {
		container[6].setZ(2);
		CHECK(container[6].getZ() == stack[5].getZ());
		container[6].setZ(3);
		CHECK(container[6].getZ() == stack[6].getZ());
		container[6].setZ(7);
		CHECK(container[6].getZ() == stack[9].getZ());
		container[6].setZ(12);
		CHECK(container[6].getZ() == stack[9].getZ());
		container[6].setZ(0);
		CHECK(container[6].getZ() == stack[2].getZ());
	}

	SUBCASE("Bound testing") {
		container[6].setZ(-123);
		CHECK(container[6].getZ() == stack[0].getZ());

		container[6].setZ(MaxStackNum);
		CHECK(container[6].getZ() == stack[12].getZ());
	}

	SUBCASE("Exact value testing") {
		container[6].setZ(5);
		CHECK(container[6].getZ() == stack[8].getZ());
	}

	SUBCASE("Push a new value") {
		container[6].setZ(5);
		auto drawable = DrawableTest::create(stack);
		CHECK(drawable.getZ() == stack[13].getZ());
	}
}
