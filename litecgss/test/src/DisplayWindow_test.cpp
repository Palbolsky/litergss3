#include <doctest.h>

#include "Views/DisplayWindow.h"

struct WindowDataTest {
	lr::DisplayWindowSettings settings{};
	lr::MainEventDispatcher eventDispatcher{};
};

static inline WindowDataTest& PrepareTest() {
	static auto data = WindowDataTest{};
	return data;
}

static inline lr::DisplayWindow CreateWindow(WindowDataTest& data = PrepareTest()) {
	return lr::DisplayWindow{ data.eventDispatcher, data.settings };
}

TEST_CASE("[DisplayWindow] Creation") {
	CreateWindow();
}

TEST_CASE("[DisplayWindow] Draw") {
	CreateWindow().draw();
}

TEST_CASE("[DisplayWindow] Window settings change event emitted") {
	auto& data = PrepareTest();
	auto window = CreateWindow(data);
	
	auto event = lr::WindowSettingsChangeEvent{};
	event.settings = data.settings;
	event.settings.video.scale = 4.0;
	data.eventDispatcher.lr::Observable<lr::WindowSettingsChangeEvent>::notifyObservers(event);
}
