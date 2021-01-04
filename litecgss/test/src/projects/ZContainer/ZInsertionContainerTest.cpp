#include <gtest/gtest.h>

#include "Views/Stack/ZInsertionContainer.h"

namespace cgss {
	template<>
	struct ZGetter<ZIndex> {
		static const ZIndex& z(const ZIndex& itm) {
			return itm;
		}

		static void setZ(ZIndex& itm, long z) {
			itm.z = z;
		}
	};
}

TEST(ZInsertionContainerGroup, Init) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	EXPECT_EQ(stack.size(), 13);
	EXPECT_EQ(stack[0], (cgss::ZIndex { 0, 0 }));
	EXPECT_EQ(stack[1], (cgss::ZIndex { 0, 0 }));
	EXPECT_EQ(stack[2], (cgss::ZIndex { 1, 0 }));
	EXPECT_EQ(stack[3], (cgss::ZIndex { 1, 0 }));
	EXPECT_EQ(stack[4], (cgss::ZIndex { 2, 0 }));
	EXPECT_EQ(stack[5], (cgss::ZIndex { 3, 0 }));
	EXPECT_EQ(stack[6], (cgss::ZIndex { 3, 0 }));
	EXPECT_EQ(stack[7], (cgss::ZIndex { 4, 0 }));
	EXPECT_EQ(stack[8], (cgss::ZIndex { 5, 0 }));
	EXPECT_EQ(stack[9], (cgss::ZIndex { 6, 0 }));
	EXPECT_EQ(stack[10], (cgss::ZIndex { 18, 0 }));
	EXPECT_EQ(stack[11], (cgss::ZIndex { 20, 0 }));
	EXPECT_EQ(stack[12], (cgss::ZIndex { 30, 0 }));
}

TEST(ZInsertionContainerGroup, NoUpdate) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	
	// Direct ZIndex access => no update supported, be careful !
	stack[6].z = -123;

	// So, still the same order as initial, but with a modified value
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, -123, 4, 5, 6, 18, 20, 30}));
}

TEST(ZInsertionContainerGroup, Update) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	
	stack.set(stack[6], 0);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 18, 20, 30}));
}

TEST(ZInsertionContainerGroup, InternalRandomMovements) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };

	stack.set(stack[6], 2);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 18, 20, 30}));

	stack.set(stack[6], 3);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 18, 20, 30}));

	stack.set(stack[6], 7);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 4, 5, 6, 7, 18, 20, 30}));

	stack.set(stack[6], 12);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 5, 6, 7, 12, 18, 20, 30}));

	stack.set(stack[6], 0);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 0, 1, 1, 2, 2, 6, 7, 12, 18, 20, 30}));
}

TEST(ZInsertionContainerGroup, Bounds) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	stack.set(stack[6], -123);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { -123, 0, 0, 1, 1, 2, 3, 4, 5, 6, 18, 20, 30}));

	stack.set(stack[6], 99999);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { -123, 0, 0, 1, 1, 2, 4, 5, 6, 18, 20, 30, 99999}));
}

TEST(ZInsertionContainerGroup, ExactExistingValue) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	stack.set(stack[6], 5);
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 4, 5, 5, 6, 18, 20, 30}));

	// stack[6] has been moved to position 8
	EXPECT_EQ(stack[7], (cgss::ZIndex{5, 0}));

	// No index change
	EXPECT_EQ(stack[8], (cgss::ZIndex{5, 0}));
}

TEST(ZInsertionContainerGroup, AddFromEmpty) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { };
	EXPECT_EQ(stack.size(), 0);
	stack.add(cgss::ZIndex { 10, 0 });
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 10 }));

	stack.add(cgss::ZIndex { 0, 0 });
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 10 }));

	stack.add(cgss::ZIndex { 10, 1 });
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, {10, 0}, {10, 1} }));

	stack.add(cgss::ZIndex { 5, 0 });
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 5, {10, 0}, {10, 1} }));
}

TEST(ZInsertionContainerGroup, Remove) {
	auto stack = cgss::ZInsertionContainer<cgss::ZIndex> { { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30} };
	stack.remove(cgss::ZIndex{ 0 }, [](const cgss::ZIndex& index) {
		return true;
	});
}
