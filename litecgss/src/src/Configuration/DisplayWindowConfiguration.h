#ifndef LR_DISPLAY_WINDOW_CONFIGURATION_H
#define LR_DISPLAY_WINDOW_CONFIGURATION_H

#include <SFML/Graphics.hpp>

namespace lr {
	struct DisplayWindowVideoConfig {
		sf::VideoMode vmode;
		unsigned int width = 640;
		unsigned int height = 480;
		double scale = 1.0;
	};

	struct DisplayWindowConfig {
		DisplayWindowVideoConfig video;
		bool smoothScreen = false;
		sf::String title;
		unsigned int frameRate = 60;
		bool vSync = false;
		bool fullscreen = false;
	};
}
#endif