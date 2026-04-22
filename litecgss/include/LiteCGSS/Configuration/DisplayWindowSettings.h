#ifndef CGSS_DISPLAY_WINDOW_SETTINGS_H
#define CGSS_DISPLAY_WINDOW_SETTINGS_H

#include <string>
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"

namespace cgss {

	namespace detail {
		struct DisplayWindowVideoSettingsData {
			unsigned int bitsPerPixel = 32;
			unsigned int width = 320;
			unsigned int height = 240;
			double scale = 2.0;
		};
	}

	struct DisplayWindowVideoSettings {
		DisplayWindowVideoSettings (
			long bitsPerPixel,
			long width,
			long height,
			double scale);
		unsigned int bitsPerPixel;
		unsigned int width;
		unsigned int height;
		double scale;
		DisplayWindowVideoSettings();
	private:
		DisplayWindowVideoSettings(detail::DisplayWindowVideoSettingsData source);
	};

	struct DisplayWindowSettings {
		bool headless = false;
		DisplayWindowVideoSettings video {};
		DisplayWindowContextSettings context {};
		bool smoothScreen = false;
		/* UTF-8 encoded title. */
		std::string title;
		unsigned int frameRate = 60;
		bool vSync = false;
		bool fullscreen = false;
		bool visibleMouse = false;
	};
}
#endif