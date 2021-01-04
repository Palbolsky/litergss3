#include <gtest/gtest.h>
#include "Common/Bindable.h"
#include "Common/BondElement.h"

struct BondTest :
	public cgss::BondElement<int> {
};

struct BindableTest :
	public cgss::Bindable<int> {
	void updateFromValue(const int* ptr) override { noValue = ptr == nullptr; if (ptr != nullptr) { updatedValue = *ptr; } else { updatedValue = 0; } };
	int updatedValue = 0;
	bool noValue = true;
};

TEST(BindableGroup, Init) {
	auto element = BindableTest {};

	// TODO
}

TEST(BondGroup, Init) {
	auto element = BondTest {};

	// TODO
}
