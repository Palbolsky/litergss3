#include <gtest/gtest.h>

#include "Graphics/ZIndex.h"

TEST(ZIndexGroup, Init) {
	{
		const auto element = cgss::ZIndex { };
		EXPECT_EQ(element.z, 0);
		EXPECT_EQ(element.index, 0);
	}

	{
		const auto element = cgss::ZIndex { 1 };
		EXPECT_EQ(element.z, 1);
		EXPECT_EQ(element.index, 0);
	}

	{
		const auto element = cgss::ZIndex { 1, 2 };
		EXPECT_EQ(element.z, 1);
		EXPECT_EQ(element.index, 2);
	}
}

TEST(ZIndexGroup, Equality) {
	const auto element = cgss::ZIndex { 1, 2 };
	EXPECT_EQ(element, (cgss::ZIndex{ 1, 2 }));
	EXPECT_FALSE(element == (cgss::ZIndex{ 1, 0 }));
	EXPECT_FALSE(element == (cgss::ZIndex{ 0, 2 }));
	EXPECT_FALSE(element == (cgss::ZIndex{ 123, 123 }));
	EXPECT_TRUE(element != (cgss::ZIndex{ 123, 123 }));
	EXPECT_TRUE(element != (cgss::ZIndex{ 1, 0 }));
	EXPECT_TRUE(element != (cgss::ZIndex{ 0, 2 }));
}

TEST(ZIndexGroup, Inferiority) {
	const auto element = cgss::ZIndex { 1, 2 };
	EXPECT_LE(element, (cgss::ZIndex{ 1, 2 }));
	EXPECT_LT(element, (cgss::ZIndex{ 2, 2 }));
	EXPECT_LT(element, (cgss::ZIndex{ 1, 3 }));
	EXPECT_FALSE(element < (cgss::ZIndex{ 1, 2 }));
	EXPECT_FALSE(element < (cgss::ZIndex{ 0, 2 }));
	EXPECT_FALSE(element < (cgss::ZIndex{ 1, 1 }));
}

TEST(ZIndexGroup, Superiority) {
	const auto element = cgss::ZIndex { 2, 2 };
	EXPECT_GE(element, (cgss::ZIndex{ 1, 2 }));
	EXPECT_GT(element, (cgss::ZIndex{ 1, 2 }));
	EXPECT_GT(element, (cgss::ZIndex{ 2, 1 }));
	EXPECT_FALSE(element > (cgss::ZIndex{ 2, 2 }));
	EXPECT_FALSE(element > (cgss::ZIndex{ 3, 2 }));
	EXPECT_FALSE(element > (cgss::ZIndex{ 2, 3 }));
}

TEST(ZIndexGroup, Substraction) {
	const auto element = cgss::ZIndex { 2, 2 };
	EXPECT_EQ((element - cgss::ZIndex{ }), element);
	EXPECT_EQ((element - cgss::ZIndex{ 1, 0 }), (cgss::ZIndex{ 1, 2 }));
	EXPECT_EQ((element - cgss::ZIndex{ 0, 1 }), (cgss::ZIndex{ 2, 1 }));
	EXPECT_EQ((element - cgss::ZIndex{ 1, 1 }), (cgss::ZIndex{ 1, 1 }));
}

TEST(ZIndexGroup, Addition) {
	const auto element = cgss::ZIndex { 2, 2 };
	EXPECT_EQ((element + cgss::ZIndex{ }), element);
	EXPECT_EQ((element + cgss::ZIndex{ 1, 0 }), (cgss::ZIndex{ 3, 2 }));
	EXPECT_EQ((element + cgss::ZIndex{ 0, 1 }), (cgss::ZIndex{ 2, 3 }));
	EXPECT_EQ((element + cgss::ZIndex{ 1, 1 }), (cgss::ZIndex{ 3, 3 }));
}

TEST(ZIndexGroup, Sign) {
	EXPECT_EQ((cgss::ZIndex { 2, 2 }).sign(), 1);
	EXPECT_EQ((cgss::ZIndex { -2, -2 }).sign(), -1);
	EXPECT_EQ((cgss::ZIndex { 0 }).sign(), 0);
	EXPECT_EQ((cgss::ZIndex { -2, 2 }).sign(), -1);
	EXPECT_EQ((cgss::ZIndex { 2, -2 }).sign(), 1);
	EXPECT_EQ((cgss::ZIndex { 0, 1 }).sign(), 1);
	EXPECT_EQ((cgss::ZIndex { 0, -1 }).sign(), -1);
}
