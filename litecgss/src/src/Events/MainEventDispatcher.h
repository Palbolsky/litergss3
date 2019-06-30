#ifndef CGSS_MAIN_EVENT_DISPATCHER_H
#define CGSS_MAIN_EVENT_DISPATCHER_H

#include "Patterns/EventDispatcher.h"

#include "WindowSettingsChangeEvent.h"

namespace cgss {
	using MainEventDispatcher = 
		EventDispatcher<
			WindowSettingsChangeEvent
		>;
}

#endif