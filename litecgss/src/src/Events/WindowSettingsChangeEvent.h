#ifndef LR_WINDOW_SETTINGS_CHANGE_EVENT_H
#define LR_WINDOW_SETTINGS_CHANGE_EVENT_H

#include "Configuration/DisplayWindowSettings.h"

namespace lr {
	struct WindowSettingsChangeEvent {
		DisplayWindowSettings settings;
	};
}

#endif