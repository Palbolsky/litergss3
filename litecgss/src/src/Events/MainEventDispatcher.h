#ifndef LR_MAIN_EVENT_DISPATCHER_H
#define LR_MAIN_EVENT_DISPATCHER_H

#include "Patterns/EventDispatcher.h"

#include "WindowSettingsChangeEvent.h"

namespace lr {
	using MainEventDispatcher = 
		EventDispatcher<
			WindowSettingsChangeEvent
		>;
}

#endif