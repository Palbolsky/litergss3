#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Common/Bindable.h"
#include "Common/BondElement.h"

struct BondTest :
	public cgss::BondElement<int> {
	int& getValue() { return cgss::BondElement<int>::value(); }
	const int& getValue() const { return cgss::BondElement<int>::getValue(); }
};

struct BindableTestBasic :
	public cgss::Bindable<int> {
	void updateFromValue(const int*) override {}
	const int* getValue() const { return cgss::Bindable<int>::getValue(); }
	int* getValue() { return cgss::Bindable<int>::value(); }
};

struct BindableTest :
	public cgss::Bindable<int> {
	MOCK_METHOD(void, updateFromValue, (const int* ptr), (override));
	const int* getValue() const { return cgss::Bindable<int>::getValue(); }
	int* getValue() { return cgss::Bindable<int>::value(); }
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

	// On BondElement destruction, nullptr is bound to Bindable
	EXPECT_CALL(element, updateFromValue(Eq(nullptr))).Times(1);
}

TEST(BindableGroup, AutomaticUnbindValueOnDestruction) {
	auto boundValue = BondTest {};
	boundValue.setValue(123);
	{
		auto element = BindableTest {};
		element.bindValue(&boundValue);

		// Automatic unbind here, in Bindable destruction
	}
	
	// Bound value is still usable after Bindable destruction
	EXPECT_EQ(boundValue.getValue(), 123);
}

TEST(BindableGroup, SetValueWithoutBondElement) {
	auto element = BindableTest {};
	
	EXPECT_CALL(element, updateFromValue(_)).Times(1);
	element.setValue(123);

	// Still, the value will stay at nullptr, because nothing is bound
	EXPECT_EQ(element.getValue(), nullptr);
}

TEST(BindableGroup, SwitchBond) {
	auto element = BindableTest {};

	auto boundValue = BondTest {};
	boundValue.setValue(1);
	boundValue.bind(&element);

	EXPECT_EQ(*element.getValue(), 1);

	auto boundValue2 = BondTest {};
	boundValue2.setValue(2);
	boundValue2.bind(&element);

	EXPECT_EQ(*element.getValue(), 2);	
}

TEST(BindableGroup, Move) {
	auto element = BindableTestBasic {};
	auto boundValue = BondTest {};
	boundValue.setValue(123);
	element.bindValue(&boundValue);

	auto element2 = BindableTestBasic {};
	auto boundValue2 = BondTest {};
	boundValue2.setValue(321);
	element2.bindValue(&boundValue2);

	element = std::move(element2);
	EXPECT_EQ(*element.getValue(), 321);

	auto element3 = BindableTestBasic { std::move(element) };
	EXPECT_EQ(*element3.getValue(), 321);
}

TEST(BondGroup, BindValue) {
	auto element = BindableTest {};

	auto boundValue = BondTest {};
	boundValue.setValue(123);

	element.bindValue(&boundValue);
	element.setValue(99);

	EXPECT_EQ(boundValue.getValue(), 99);
}

TEST(BondGroup, GetValue) {
	auto boundValue = BondTest {};
	boundValue.setValue(123);

	EXPECT_EQ(boundValue.getValue(), 123);
}

TEST(BondGroup, Equality) {
	auto boundValue = BondTest {};
	boundValue.setValue(123);

	auto boundValue2 = BondTest {};
	boundValue2.setValue(100);

	auto boundValue3 = BondTest {};
	boundValue3.setValue(123);

	EXPECT_TRUE(boundValue != boundValue2);
	EXPECT_TRUE(boundValue == boundValue3);
}

TEST(BondGroup, MoveValues) {
	auto boundValue = BondTest {};
	boundValue.setValue(123);

	auto boundValue2 = BondTest {};
	boundValue2.setValue(100);

	// Operator
	boundValue = std::move(boundValue2);
	EXPECT_EQ(boundValue.getValue(), 100);

	// Constructor
	auto boundValue3 = BondTest{std::move(boundValue)};
	EXPECT_EQ(boundValue3.getValue(), 100);
}

TEST(BondGroup, MoveWithBindable) {
	auto element = BindableTest {};

	auto boundValue = BondTest {};
	boundValue.setValue(1);
	{
		auto boundValue2 = BondTest {};
		boundValue2.setValue(2);
		boundValue2.bind(&element);

		boundValue = std::move(boundValue2);
	}

	EXPECT_EQ(boundValue.getValue(), 2);
	EXPECT_EQ(*element.getValue(), 2);
}

TEST(BondGroup, GetValueConst) {
	auto boundValue = BondTest {};
	boundValue.setValue(1);
	EXPECT_EQ(static_cast<const BondTest&>(boundValue).getValue(), 1);
}

TEST(BondGroup, EditValue) {
	auto element = BindableTest {};
	auto boundValue = BondTest {};
	boundValue.bind(&element);

	EXPECT_CALL(element, updateFromValue(&boundValue.getValue())).Times(1);
	boundValue.edit([] (int& value) {
		value = 123;
	});

	EXPECT_EQ(boundValue.getValue(), 123);

	EXPECT_CALL(element, updateFromValue(nullptr)).Times(1);
}
