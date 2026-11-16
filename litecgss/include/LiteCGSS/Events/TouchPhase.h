#ifndef CGSS_TOUCH_PHASE_H
#define CGSS_TOUCH_PHASE_H

// Touch event phase. Backends whose native touch API has no phase
// synthesize it from touch-ID set diffs in their event queue.
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
