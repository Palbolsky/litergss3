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

TEST(ZFullContainerGroup, Remove) {
	const auto init = std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30};
	auto stack = cgss::ZFullContainer<cgss::ZIndex> { init };

	stack.removeOne(cgss::ZIndex { 3 }, [](const cgss::ZIndex&) {
		return true;
	});

	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 4, 5, 6, 18, 20, 30}));
}

TEST(ZFullContainerGroup, RemoveUnsorted) {
	const auto init = std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30};
	auto stack = cgss::ZFullContainer<cgss::ZIndex> { init };

	// Sets the dirty flag
	stack.set(stack[6], 2);

	// No refresh required after a remove, because : 1 - it's automatically done before, 2 - a remove does not change the order
	// (the remove operation is performed through binary lookup which require the container to be sorted)
	stack.removeOne(cgss::ZIndex { 3 }, [](const cgss::ZIndex&) {
		return true;
	});

	// No refresh required after a remove, because : 1 - it's automatically done before, 2 - a remove does not change the order
	// (the remove operation is performed through binary lookup which require the container to be sorted)
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 2, 4, 5, 6, 18, 20, 30}));
}

TEST(ZFullContainerGroup, Add) {
	const auto init = std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30};
	auto stack = cgss::ZFullContainer<cgss::ZIndex> { init };

	// Sets the dirty flag
	stack.add(cgss::ZIndex { 7 });

	// Not sorted
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 18, 20, 30, 7}));

	stack.refresh();

	// Sorted
	EXPECT_TRUE((stack == std::vector<cgss::ZIndex> { 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 18, 20, 30}));
}
