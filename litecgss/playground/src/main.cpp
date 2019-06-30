#include <iostream>

#include "Views/DisplayWindow.h"
#include "Configuration/DisplayWindowSettings.h"

int main(int argc, char* argv[]) {
	auto settings = cgss::DisplayWindowSettings{};
	auto eventDispatcher = cgss::MainEventDispatcher{};

	auto window = cgss::DisplayWindow{ eventDispatcher, settings };
	window.draw();

	auto event = cgss::WindowSettingsChangeEvent{};
	event.settings = settings;
	event.settings.video.scale = 4.0;
	eventDispatcher.cgss::Observable<cgss::WindowSettingsChangeEvent>::notifyObservers(event);



	return 0;
}
