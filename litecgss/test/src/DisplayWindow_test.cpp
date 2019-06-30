#include <doctest.h>

#include "Views/DisplayWindow.h"

struct WindowDataTest {
	cgss::DisplayWindowSettings settings{};
	cgss::MainEventDispatcher eventDispatcher{};
};

static inline WindowDataTest& PrepareTest() {
	static auto data = WindowDataTest{};
	return data;
}

static inline cgss::DisplayWindow CreateWindow(WindowDataTest& data = PrepareTest()) {
	return cgss::DisplayWindow{ data.settings };
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
	
	auto event = cgss::WindowSettingsChangeEvent{};
	event.settings = data.settings;
	event.settings.video.scale = 4.0;
	data.eventDispatcher.cgss::Observable<cgss::WindowSettingsChangeEvent>::notifyObservers(event);
}
