#ifndef CGSS_WINDOW_SETTINGS_CHANGE_EVENT_H
#define CGSS_WINDOW_SETTINGS_CHANGE_EVENT_H

namespace cgss {
	struct WindowSizeChangeEvent {
		unsigned int width;
		unsigned int height;
		/* Where the game's (0, 0) is on the window (see DisplayWindowSettings::originX) */
		long originX = 0;
		long originY = 0;
	};
}

#endif