#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "LiteCGSS/Common/Rectangle.h"

struct BindableToRectangle :
	public cgss::Bindable<cgss::IntRect> {
	MOCK_METHOD(void, updateFromValue, (const cgss::IntRect* ptr), (override));
	const cgss::IntRect* getValue() const { return cgss::Bindable<cgss::IntRect>::getValue(); }
};

TEST(RectangleGroup, Init) {
	auto rectangle = cgss::Rectangle {};
	const auto defaultRect = cgss::IntRect{};
	EXPECT_EQ(static_cast<const cgss::Rectangle&>(rectangle).getValue(), defaultRect);
}

TEST(RectangleGroup, SetX) {
	auto rectangle = cgss::Rectangle{};

	rectangle.setX(1);
	EXPECT_EQ(static_cast<const cgss::Rectangle&>(rectangle).getValue(), (cgss::IntRect{1, 0, 0, 0}));
}

TEST(RectangleGroup, SetY) {
	auto rectangle = cgss::Rectangle{};

	rectangle.setY(1);
	EXPECT_EQ(static_cast<const cgss::Rectangle&>(rectangle).getValue(), (cgss::IntRect{0, 1, 0, 0}));
}

TEST(RectangleGroup, SetWidth) {
	auto rectangle = cgss::Rectangle{};

	rectangle.setWidth(1);
	EXPECT_EQ(static_cast<const cgss::Rectangle&>(rectangle).getValue(), (cgss::IntRect{0, 0, 1, 0}));
}

TEST(RectangleGroup, SetHeight) {
	auto rectangle = cgss::Rectangle{};

	rectangle.setHeight(1);
	EXPECT_EQ(static_cast<const cgss::Rectangle&>(rectangle).getValue(), (cgss::IntRect{0, 0, 0, 1}));
}

TEST(RectangleGroup, BindableSetX) {
	auto rectangle = cgss::Rectangle{};
	auto bindable = BindableToRectangle{};
	rectangle.bind(&bindable);
	EXPECT_CALL(bindable, updateFromValue(&static_cast<const cgss::Rectangle&>(rectangle).getValue())).Times(1);
	rectangle.setX(1);
	EXPECT_EQ(*static_cast<const BindableToRectangle&>(bindable).getValue(), (cgss::IntRect{1, 0, 0, 0}));
}

TEST(RectangleGroup, BindableSetY) {
	auto rectangle = cgss::Rectangle{};
	auto bindable = BindableToRectangle{};
	rectangle.bind(&bindable);
	EXPECT_CALL(bindable, updateFromValue(&static_cast<const cgss::Rectangle&>(rectangle).getValue())).Times(1);
	rectangle.setY(1);
	EXPECT_EQ(*static_cast<const BindableToRectangle&>(bindable).getValue(), (cgss::IntRect{0, 1, 0, 0}));
}

TEST(RectangleGroup, BindableSetWidth) {
	auto rectangle = cgss::Rectangle{};
	auto bindable = BindableToRectangle{};
	rectangle.bind(&bindable);
	EXPECT_CALL(bindable, updateFromValue(&static_cast<const cgss::Rectangle&>(rectangle).getValue())).Times(1);
	rectangle.setWidth(1);
	EXPECT_EQ(*static_cast<const BindableToRectangle&>(bindable).getValue(), (cgss::IntRect{0, 0, 1, 0}));
}

TEST(RectangleGroup, BindableSetHeight) {
	auto rectangle = cgss::Rectangle{};
	auto bindable = BindableToRectangle{};
	rectangle.bind(&bindable);
	EXPECT_CALL(bindable, updateFromValue(&static_cast<const cgss::Rectangle&>(rectangle).getValue())).Times(1);
	rectangle.setHeight(1);
	EXPECT_EQ(*static_cast<const BindableToRectangle&>(bindable).getValue(), (cgss::IntRect{0, 0, 0, 1}));
}
