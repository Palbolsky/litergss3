#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Common/Bindable.h"
#include "Common/BondElement.h"

struct BondTest :
	public cgss::BondElement<int> {
	int& getValue() { return cgss::BondElement<int>::getValue(); }
};

struct BindableTest :
	public cgss::Bindable<int> {
	MOCK_METHOD(void, updateFromValue, (const int* ptr), (override));
	const int* getValue() const { return cgss::Bindable<int>::getValue(); }
	int* getValue() { return cgss::Bindable<int>::getValue(); }
};

using ::testing::Return;
using ::testing::Eq;
using ::testing::_;

TEST(BindableGroup, Init) {
	auto element = BindableTest {};

	EXPECT_EQ(element.getValue(), nullptr);
	EXPECT_EQ(static_cast<const BindableTest&>(element).getValue(), nullptr);
}

TEST(BindableGroup, BindValueScope) {
	auto element = BindableTest {};

	{
		auto boundValue = BondTest {};
		EXPECT_CALL(element, updateFromValue(_)).Times(1);
		element.bindValue(&boundValue);
		
		EXPECT_EQ(element.getValue(), (&boundValue.getValue()));
		EXPECT_EQ(static_cast<const BindableTest&>(element).getValue(), (&boundValue.getValue()));

		EXPECT_CALL(element, updateFromValue(_)).Times(1);
	}

	EXPECT_EQ(element.getValue(), nullptr);
	EXPECT_EQ(static_cast<const BindableTest&>(element).getValue(), nullptr);
}

TEST(BindableGroup, BindValue) {
	auto element = BindableTest {};

	auto boundValue = BondTest {};
	boundValue.setValue(123);

	EXPECT_CALL(element, updateFromValue(Eq(&boundValue.getValue()))).Times(2);
	// Once here
	element.bindValue(&boundValue);

	// Twice here
	boundValue.setValue(99);

	EXPECT_EQ(*element.getValue(), 99);

	EXPECT_CALL(element, updateFromValue(Eq(nullptr))).Times(1);
}

TEST(BondGroup, BindValue) {
	auto element = BindableTest {};

	auto boundValue = BondTest {};
	boundValue.setValue(123);

	element.bindValue(&boundValue);
	element.setValue(99);

	EXPECT_EQ(boundValue.getValue(), 99);
}
