#include <doctest.h>

#include "Views/FramedView.h"

struct FramedViewDataTest {
	cgss::DisplayWindowSettings settings{};
	cgss::MainEventDispatcher eventDispatcher{};
};

static inline FramedViewDataTest& PrepareTest() {
	static auto data = FramedViewDataTest{};
	return data;
}

static inline cgss::FramedView CreateFramedView(FramedViewDataTest& data = PrepareTest()) {
	return cgss::FramedView{ data.eventDispatcher, data.settings };
}

TEST_CASE("[FramedView] Creation") {
	CreateFramedView();
}

TEST_CASE("[FramedView] Bind rectangle") {
	auto framedView = CreateFramedView();
	framedView.bindRectangle(nullptr);
}
