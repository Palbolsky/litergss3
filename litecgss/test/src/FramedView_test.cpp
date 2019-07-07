#include <doctest.h>

#include "Views/FramedView.h"
#include "Views/DisplayWindow.h"

struct FramedViewDataTest {
	cgss::DisplayWindowSettings settings{};
	cgss::MainEventDispatcher eventDispatcher{};
	cgss::DisplayWindow window {settings};
};

static inline FramedViewDataTest& PrepareTest() {
	static auto data = FramedViewDataTest{};
	data.window.clearViews();
	return data;
}

static inline cgss::FramedView& CreateFramedView(FramedViewDataTest& data = PrepareTest()) {
	return data.window.addView<cgss::FramedView>();
}

TEST_CASE("[FramedView] Creation") {
	CreateFramedView();
}

TEST_CASE("[FramedView] Lock") {
	auto& framedView = CreateFramedView();
	CHECK(!framedView.isLocked());
	framedView.lock();
	CHECK(framedView.isLocked());
}

TEST_CASE("[FramedView] Unlock") {
	auto& framedView = CreateFramedView();
	framedView.lock();
	framedView.unlock();
	CHECK(!framedView.isLocked());
}
