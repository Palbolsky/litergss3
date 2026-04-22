#ifndef CGSS_MAIN_EVENT_DISPATCHER_H
#define CGSS_MAIN_EVENT_DISPATCHER_H

#include "LiteCGSS/Events/Patterns/EventDispatcher.h"

#include "LiteCGSS/Events/WindowSizeChangeEvent.h"

namespace cgss {
	using MainEventDispatcher =
		EventDispatcher<
			WindowSizeChangeEvent
		>;
}

#endif