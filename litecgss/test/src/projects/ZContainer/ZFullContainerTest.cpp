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
