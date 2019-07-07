#include <iostream>

#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Configuration/DisplayWindowSettings.h"

int main(int argc, char* argv[]) {
	auto settings = cgss::DisplayWindowSettings{};

	auto window = cgss::DisplayWindow{ settings };
	window.draw();

	window.addView<cgss::Viewport>();

	auto event = cgss::WindowSettingsChangeEvent{};
	event.settings = settings;
	event.settings.video.scale = 4.0;
	window.reload(event.settings);

	return 0;
}
