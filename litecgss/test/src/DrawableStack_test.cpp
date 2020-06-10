#include <doctest.h>

#include "Views/Stack/DrawableStack.h"
#include "Graphics/Drawable.h"

struct DrawableTest;
struct DrawableStackItemTest : 
	public cgss::GraphicsStackItem {
	DrawableStackItemTest(DrawableTest*) {}
	uint32_t id = 0;
protected:
	void draw(sf::RenderTarget& target) const override {};
	void drawFast(sf::RenderTarget& target) const override {};
};

struct DrawableTest : 
	public cgss::Drawable<DrawableTest, DrawableStackItemTest> {
	bool hasProxy() const { return cgss::Drawable<DrawableTest, DrawableStackItemTest>::hasProxy(); }
	void setId(std::size_t id) { m_proxy->id = id; }
};


TEST_CASE("[DrawableStack] DrawableStack : drawable dies before : no effect") {
	auto stack = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		CHECK(stack.size() == 1);
	}

	CHECK(stack.size() == 1);
}

TEST_CASE("[DrawableStack] DrawableStack : drawable disposed before : empty stack") {
	auto stack = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		sprite.dispose();
	}

	CHECK(stack.size() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : stack dies before (moved Drawable)") {
	auto sprite2 = DrawableTest{};
	{
		auto stack = cgss::DrawableStack{};
		auto sprite = DrawableTest::create(stack);
		sprite2 = std::move(sprite);
		CHECK(stack.size() == 1);
	}

	// Still usable safely, but no more presence on screen
	CHECK(sprite2.hasProxy());
}

TEST_CASE("[DrawableStack] DrawableStack : drawable is disposed explicitely") {
	auto stack = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		CHECK(stack.size() == 1);
		sprite.dispose();
		CHECK(stack.size() == 0);
	}

	CHECK(stack.size() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : moved sprite is not disposed") {
	auto stack = cgss::DrawableStack{};
	
	auto sprite = DrawableTest::create(stack);
	auto sprite2 = DrawableTest::create(stack);
	CHECK(stack.size() == 2);
	sprite = std::move(sprite2);
	CHECK(stack.size() == 2);
}

TEST_CASE("[DrawableStack] DrawableStack : dispose several times") {
	auto stack = cgss::DrawableStack{};

	auto sprite = DrawableTest::create(stack);
	CHECK(stack.size() == 1);
	sprite.dispose();
	CHECK(stack.size() == 0);
	sprite.dispose();
	CHECK(stack.size() == 0);
}

TEST_CASE("[DrawableStack] DrawableStack : complex mix") {
	auto stack = cgss::DrawableStack{};
	auto stack2 = cgss::DrawableStack{};

	{
		auto sprite = DrawableTest::create(stack);
		auto sprite2 = DrawableTest::create(stack);

		CHECK(stack.size() == 2);

		sprite2.dispose();

		CHECK(stack.size() == 1);

		auto sprite3 = DrawableTest::create(stack2);

		CHECK(stack2.size() == 1);

		sprite = std::move(sprite3);

		CHECK(stack.size() == 1);
		CHECK(stack2.size() == 1);
	}

	CHECK(stack.size() == 1);
	CHECK(stack2.size() == 1);

}

TEST_CASE("[DrawableStack] DrawableStack with drawing") {
	auto stack = cgss::DrawableStack{};

	auto renderTarget = sf::RenderWindow{};

	auto sprite2 = DrawableTest::create(stack);
	CHECK(sprite2.hasProxy());

	// Here sprite2 is deleted from stack
	sprite2.dispose();
	
	CHECK(stack.size() == 0);
	
	// We still can use the object safely, it just has no effects anymore
	auto view = renderTarget.getView();
	stack.draw(view, renderTarget);

	CHECK(sprite2.hasProxy());
}


TEST_CASE("[DrawableStack] Default z index") {
	auto stack = cgss::DrawableStack{};
	auto sprite = DrawableTest::create(stack);
	CHECK(sprite.getZ().z == 0);
	CHECK(sprite.getZ().index == 0);
	
	auto sprite2 = DrawableTest::create(stack);
	CHECK(sprite2.getZ().z == 0);
	CHECK(sprite2.getZ().index == 1);
}

TEST_CASE("[DrawableStack] z sorting") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30 };
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

		container[6].setZ(99999);
		CHECK(container[6].getZ() == stack[12].getZ());
	}

	SUBCASE("Exact value testing") {
		container[6].setZ(5);
		CHECK(container[6].getZ() == stack[7].getZ());
	}

	SUBCASE("Push a new value") {
		container[6].setZ(5);
		auto drawable = DrawableTest::create(stack);
		CHECK(drawable.getZ() == stack[2].getZ());
	}
}


TEST_CASE("[DrawableStack] z sorting from scratch") {
	constexpr const auto MaxStackNum = std::numeric_limits<long>::max();

	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 2000, 2000, 2000, 0, 1 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		index++;
	}
	
	CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[0]).id == 3);
	CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[1]).id == 4);
	CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[2]).id == 0);
	CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[3]).id == 1);
	CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[4]).id == 2);

	CHECK(stack[0].getZ() == cgss::ZIndex{ 0, 3 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 1, 4 });
	CHECK(stack[2].getZ() == cgss::ZIndex{ 2000, 0 });
	CHECK(stack[3].getZ() == cgss::ZIndex{ 2000, 1 });
	CHECK(stack[4].getZ() == cgss::ZIndex{ 2000, 2 });
}
