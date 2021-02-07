#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Graphics/Drawable.h"
#include "Common/Rectangle.h"

struct DrawableTest;

struct StackItemTest :
	public cgss::GraphicsStackItem {
	StackItemTest(DrawableTest*){}
	MOCK_METHOD(void, draw, (sf::RenderTarget& target), (const override));
    MOCK_METHOD(void, drawFast, (sf::RenderTarget& target), (const override));
};

struct ViewTest {
	template <class View, class StackItem, class DrawableOwner>
	void add(std::shared_ptr<StackItem> a) {
		added = std::move(a);
	}
	std::shared_ptr<StackItemTest> added {};
};

struct DrawableTest : 
	public cgss::Drawable<DrawableTest, StackItemTest> {
	DrawableTest() = default;
	DrawableTest(DrawableTest&&) = default;
	virtual ~DrawableTest() = default;

	bool hasProxy() const {
		return cgss::Drawable<DrawableTest, StackItemTest>::hasProxy();
	}

};

TEST(GraphicsGroup, DrawableInit) {
	auto drawable = DrawableTest{};
	EXPECT_FALSE(drawable.isDetached());
	EXPECT_FALSE(drawable.hasProxy());
}

TEST(GraphicsGroup, DrawableCreate) {
	auto view = ViewTest{};
	auto drawableOwner = DrawableTest::create<ViewTest>(view);
	EXPECT_TRUE(view.added != nullptr);
	EXPECT_TRUE(view.added == drawableOwner.weak().lock());
}

TEST(GraphicsGroup, DrawableZ) {
	auto view = ViewTest{};
	auto drawableOwner = DrawableTest::create<ViewTest>(view);
	drawableOwner.setZ(999);
	EXPECT_TRUE(drawableOwner.getZ() == 999);
}

TEST(GraphicsGroup, DrawableDetach) {
	auto stack = cgss::DrawableStack{};
	auto drawableOwner = DrawableTest::create(stack);

	EXPECT_TRUE(stack.size() == 1);

	drawableOwner.detach();

	EXPECT_TRUE(stack.size() == 0);
}

TEST(GraphicsGroup, DrawableVisible) {
	auto stack = cgss::DrawableStack{};
	auto drawableOwner = DrawableTest::create(stack);

	EXPECT_TRUE(drawableOwner.isVisible());

	drawableOwner.setVisible(false);

	EXPECT_FALSE(drawableOwner.isVisible());
}

TEST(GraphicsGroup, DrawableBindRectangle) {
	auto view = ViewTest{};
	auto drawableOwner = DrawableTest::create<ViewTest>(view);

	auto rect = cgss::Rectangle{};
	drawableOwner.bindRectangle(&rect);
}

TEST(GraphicsGroup, DrawableMove) {
	auto stack = cgss::DrawableStack{};
	auto drawableOwner = DrawableTest::create(stack);
	
	EXPECT_TRUE(stack.size() == 1);

	auto drawableOwner2 = std::move(drawableOwner);

	EXPECT_TRUE(stack.size() == 1);
}

TEST(GraphicsItemGroup, Init) {
	auto item = StackItemTest {nullptr};
	EXPECT_TRUE(item.isVisible());
}

TEST(GraphicsItemGroup, SetZ) {
	auto item = StackItemTest {nullptr};
	EXPECT_TRUE(item.getZ() == 0);
	item.setZ(123);
	EXPECT_TRUE(item.getZ() == 123);
}
