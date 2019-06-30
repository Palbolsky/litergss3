#include <iostream>

#include "Views/DisplayWindow.h"
#include "Configuration/DisplayWindowSettings.h"

int main(int argc, char* argv[]) {
	auto settings = lr::DisplayWindowSettings{};
	auto eventDispatcher = lr::MainEventDispatcher{};

	auto window = lr::DisplayWindow{ eventDispatcher, settings };
	window.draw();

	auto event = lr::WindowSettingsChangeEvent{};
	event.settings = settings;
	event.settings.video.scale = 4.0;
	eventDispatcher.lr::Observable<lr::WindowSettingsChangeEvent>::notifyObservers(event);



	return 0;
}
