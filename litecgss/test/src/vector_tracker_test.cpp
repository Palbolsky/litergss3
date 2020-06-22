#include <doctest.h>

#include "Common/vector_tracker.h"

struct VectorTrackerElementTest {
    bool operator==(const VectorTrackerElementTest&) const { return true; }
};
using VectorTrackerTest = cgss::vector_tracker<VectorTrackerElementTest>;

TEST_CASE("[vector_tracker] push_back ") {
	auto container = VectorTrackerTest {};
    CHECK(container.size() == 0);
    CHECK(container.begin() == container.end());
    container.push_back({});
    CHECK(container.size() == 1);
    CHECK(container.begin() != container.end());
}

TEST_CASE("[vector_tracker] clear ") {
	auto container = VectorTrackerTest {};
    container.push_back({});
    CHECK(container.size() == 1);
    container.clear();
    CHECK(container.size() == 0);
    CHECK(container.begin() == container.end());
}

TEST_CASE("[vector_tracker] remove ") {
	auto container = VectorTrackerTest {};
    container.push_back({});
    CHECK(container.size() == 1);
    container.remove(VectorTrackerElementTest{});
    CHECK(container.size() == 0);
}

TEST_CASE("[vector_tracker] remove_if ") {
	auto container = VectorTrackerTest {};
    container.push_back({});
    CHECK(container.size() == 1);
    container.remove_if([] (const VectorTrackerElementTest&) {
        return true;
    });
    CHECK(container.size() == 0);
}
