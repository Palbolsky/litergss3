#include <gtest/gtest.h>

#include "Views/Stack/ZFullContainer.h"

namespace cgss {
	template<>
	struct ZGetter<ZIndex> {
		static const ZIndex& z(const ZIndex& itm) {
			return itm;
		}
	};
}

TEST(ZFullContainerGroup, RefreshTrigger) {
	const auto init = std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30};
	auto stack = cgss::ZFullContainer<cgss::ZIndex> { init };

	// Sets the dirty flag
	stack.set(stack[6], 2);

	// No refresh triggered, no sorting in full mode
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 2, 4, 5, 6, 18, 20, 30}));

	stack.refresh();
	// Refresh triggered => sorting
	const auto edited = std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 18, 20, 30};
	EXPECT_TRUE((stack == edited));

	// Direct element edit => does NOT set the dirty flag
	stack[6].z = 0;
	// Even if refreshed is called, stack is still not sorted
	stack.refresh();
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 0, 4, 5, 6, 18, 20, 30}));
}

/*
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