#ifndef CGSS_TOUCH_PHASE_H
#define CGSS_TOUCH_PHASE_H

// Touch event phase. Neither SFML 2.6 nor raylib expose phase directly
// on their touch APIs — cgss synthesizes it from touch-ID set diffs
// in the backend's event queue (see Backend/Internal/RaylibEventQueue.cpp).
// `Cancelled` is reserved for future use (e.g. gesture interruption);
// current backends never emit it.

namespace cgss {

	enum class TouchPhase : int {
		Began     = 0,
		Moved     = 1,
		Ended     = 2,
		Cancelled = 3,
	};

}

#endif
