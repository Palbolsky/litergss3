#include <doctest.h>

#include "Views/FramedView.h"

struct FramedViewDataTest {
	lr::DisplayWindowSettings settings{};
	lr::MainEventDispatcher eventDispatcher{};
};

static inline FramedViewDataTest& PrepareTest() {
	static auto data = FramedViewDataTest{};
	return data;
}

static inline lr::FramedView CreateFramedView(FramedViewDataTest& data = PrepareTest()) {
	return lr::FramedView{ data.eventDispatcher, data.settings };
}

TEST_CASE("[FramedView] Creation") {
	CreateFramedView();
}

TEST_CASE("[FramedView] Bind rectangle") {
	auto framedView = CreateFramedView();
	framedView.bindRectangle(nullptr);
}
