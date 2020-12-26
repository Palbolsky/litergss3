#include <gtest/gtest.h>
#include "Common/vector_tracker.h"

struct VectorTrackerElementTest {
	bool operator==(const VectorTrackerElementTest&) const { return true; }
};
using VectorTrackerTest = cgss::vector_tracker<VectorTrackerElementTest>;

TEST(VectorTrackerGroup, PushBack) {
	auto container = VectorTrackerTest {};
	EXPECT_EQ(container.size(), 0);
	EXPECT_EQ(container.begin(), container.end());
	container.push_back({});
	EXPECT_EQ(container.size(), 1);
	EXPECT_FALSE(container.begin() == container.end());
}

TEST(VectorTrackerGroup, Clear) {
	auto container = VectorTrackerTest {};
	container.push_back({});
	EXPECT_EQ(container.size(), 1);
	container.clear();
	EXPECT_EQ(container.size(), 0);
	EXPECT_EQ(container.begin(), container.end());
}

TEST(VectorTrackerGroup, Remove) {
	auto container = VectorTrackerTest {};
	container.push_back({});
	EXPECT_EQ(container.size(), 1);
	container.remove(VectorTrackerElementTest{});
	EXPECT_EQ(container.size(), 0);
}

TEST(VectorTrackerGroup, RemoveIf) {
	auto container = VectorTrackerTest {};
	container.push_back({});
	EXPECT_EQ(container.size(), 1);
	container.remove_if([] (const VectorTrackerElementTest&) {
		return true;
	});
	EXPECT_EQ(container.size(), 0);
}
