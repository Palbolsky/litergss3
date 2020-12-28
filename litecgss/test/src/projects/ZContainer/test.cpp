#include <gtest/gtest.h>

#include "Views/Stack/ZBaseContainer.h"

TEST(ZContainerGroup, DefaultZIndex) {
	/*auto stack = cgss::GraphicsContainer {};
	auto sprite = DrawableTest::create(stack);
	CHECK(sprite.getZ().z == 0);
	CHECK(sprite.getZ().index == 0);

	auto sprite2 = DrawableTest::create(stack);
	CHECK(sprite2.getZ().z == 0);
	CHECK(sprite2.getZ().index == 1);*/
}

/*
TEST_CASE("[DrawableStack] z sorting") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30 };
	auto container = std::vector<DrawableTest>{};
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setZ(zindexStack[index]);
		drawable.setId(index);
		container.push_back(std::move(drawable));
		index++;
	}
	stack.sortZ();
	SUBCASE("Internal 'random' movements") {
		container[6].setZ(2);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[5].getZ());
		container[6].setZ(3);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[6].getZ());
		container[6].setZ(7);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[9].getZ());
		container[6].setZ(12);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[9].getZ());
		container[6].setZ(0);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[2].getZ());
	}

	SUBCASE("Bound testing") {
		container[6].setZ(-123);
		stack.sortZ();
		std::cout << PrintStack(stack);
		CHECK(container[6].getZ() == stack[0].getZ());

		container[6].setZ(99999);
		stack.sortZ();
		std::cout << PrintStack(stack);
		CHECK(reinterpret_cast<const DrawableStackItemTest&>(stack[12]).id == 6);
		CHECK(container[6].getZ().z == stack[12].getZ().z);
	}

	SUBCASE("Exact value testing") {
		container[6].setZ(5);
		stack.sortZ();
		CHECK(container[6].getZ() == stack[7].getZ());
	}

	SUBCASE("Push a new value") {
		container[6].setZ(5);
		auto drawable = DrawableTest::create(stack);
		stack.sortZ();
		CHECK(drawable.getZ() == stack[2].getZ());
	}
}


TEST_CASE("[DrawableStack] adding elements (starting by high value = 2000)") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 2000, 1 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		index++;
	}
	stack.sortZ();
	CHECK(stack[0].getZ() == cgss::ZIndex{ 1, 1 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 2000, 0 });
}

TEST_CASE("[DrawableStack] adding elements (starting by low value = 1)") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 1, 2000 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		index++;
	}
	std::cout << PrintStack(stack);
	CHECK(stack[0].getZ() == cgss::ZIndex{ 1, 0 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 2000, 1 });
}

TEST_CASE("[DrawableStack] adding elements (starting by high value = 2000 then 0)") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 2000, 0 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		std::cout << PrintStack(stack);
		index++;
	}
	stack.sortZ();
	CHECK(stack[0].getZ() == cgss::ZIndex{ 0, 1 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 2000, 0 });
}

TEST_CASE("[DrawableStack] adding elements (starting by default value = 0)") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 0, 2000 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		index++;
	}
	stack.sortZ();
	CHECK(stack[0].getZ() == cgss::ZIndex{ 0, 0 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 2000, 1 });
}

TEST_CASE("[DrawableStack] adding elements (only default value = 0)") {
	auto stack = cgss::DrawableStack{};
	DrawableTest::create(stack);
	DrawableTest::create(stack);
	DrawableTest::create(stack);

	stack.sortZ();

	std::cout << PrintStack(stack);

	CHECK(stack[0].getZ() == cgss::ZIndex{ 0, 0 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 0, 1 });
	CHECK(stack[2].getZ() == cgss::ZIndex{ 0, 2 });
}


TEST_CASE("[DrawableStack] adding elements with default value then set it with different z") {
	auto stack = cgss::DrawableStack{};
	auto container = std::vector<DrawableTest>{};
	auto zindexStack = std::vector<long> { 2000, 0, 1, 0 };
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		container.push_back(DrawableTest::create(stack));
	}

	std::size_t index = 0;
	for (auto& drawable : container) {
		drawable.setZ(zindexStack[index]);
		std::cout << PrintStack(stack);
		index++;
	}
	stack.sortZ();

	CHECK(stack[0].getZ() == cgss::ZIndex{ 0, 1 });
	CHECK(stack[1].getZ() == cgss::ZIndex{ 0, 3 });
	CHECK(stack[2].getZ() == cgss::ZIndex{ 1, 2 });
	CHECK(stack[3].getZ() == cgss::ZIndex{ 2000, 0 });
}

TEST_CASE("[DrawableStack] remove elements") {
	auto stack = cgss::DrawableStack{};
	auto zindexStack = std::vector<long> { 2000, 0, 0, 1, 0 };
	
	std::size_t index = 0;
	for (const auto& zindex : zindexStack) {
		(void)zindex;
		auto drawable = DrawableTest::create(stack);
		drawable.setId(index);
		drawable.setZ(zindexStack[index]);
		std::cout << PrintStack(stack);
		index++;
	}

	//stack[0].detach();
}
*/