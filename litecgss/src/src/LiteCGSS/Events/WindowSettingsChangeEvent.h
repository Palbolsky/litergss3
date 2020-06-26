#ifndef CGSS_WINDOW_SETTINGS_CHANGE_EVENT_H
#define CGSS_WINDOW_SETTINGS_CHANGE_EVENT_H

#include "LiteCGSS/Configuration/DisplayWindowSettings.h"

namespace cgss {
	struct WindowSettingsChangeEvent {
		DisplayWindowSettings settings;
	};
}

#endif