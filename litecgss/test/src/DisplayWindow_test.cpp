#include <doctest.h>

#include "Views/DisplayWindow.h"

struct WindowDataTest : cgss::Observer<cgss::WindowSettingsChangeEvent> {
	WindowDataTest() :
		cgss::Observer<cgss::WindowSettingsChangeEvent>(std::bind(&WindowDataTest::onWindowSettingsChangeEvent, this, std::placeholders::_1)) {
		eventDispatcher.addObserver(*this);
	}

	bool onWindowSettingsChangeEvent(const cgss::WindowSettingsChangeEvent& event) {
		if (callback) {
			callback(event);
		}
		return true;
	}

	cgss::DisplayWindowSettings settings{ true };
	cgss::MainEventDispatcher eventDispatcher{};
	std::function<void(const cgss::WindowSettingsChangeEvent&)> callback;
};

static inline WindowDataTest& PrepareTest() {
	static auto data = WindowDataTest{};
	return data;
}

static inline cgss::DisplayWindow CreateWindow(WindowDataTest& data = PrepareTest()) {
	return cgss::DisplayWindow{ data.eventDispatcher, data.settings };
}

TEST_CASE("[DisplayWindow] Creation") {
	CreateWindow();
}

TEST_CASE("[DisplayWindow] Draw") {
	CreateWindow().draw();
}

TEST_CASE("[DisplayWindow] Window settings change event emitted") {
	auto& data = PrepareTest();
	
	bool triggered = false;
	data.callback = [&triggered](const cgss::WindowSettingsChangeEvent& event) {
		triggered = true;
	};

	auto window = CreateWindow(data);
	
	CHECK(window.screenWidth() == 320);

	CHECK(triggered);
	triggered = false;
	data.callback = [&triggered](const cgss::WindowSettingsChangeEvent& event) {
		triggered = true;
		CHECK(event.settings.video.width == 1234);
	};

	data.settings.video.width = 1234;
	window.reload(data.settings);
	CHECK(triggered);

	CHECK(window.screenWidth() == 1234);
}
