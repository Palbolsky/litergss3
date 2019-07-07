#ifndef CGSS_DISPLAY_WINDOW_SETTINGS_H
#define CGSS_DISPLAY_WINDOW_SETTINGS_H

#include <SFML/Graphics.hpp>

namespace cgss {
	struct DisplayWindowVideoSettings {
		sf::VideoMode vmode;
		unsigned int width = 320;
		unsigned int height = 240;
		double scale = 2.0;
	};

	struct DisplayWindowSettings {
		DisplayWindowVideoSettings video;
		bool smoothScreen = false;
		sf::String title;
		unsigned int frameRate = 60;
		bool vSync = false;
		bool fullscreen = false;
	};
}
#endif