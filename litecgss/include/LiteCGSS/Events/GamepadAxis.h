#ifndef CGSS_GAMEPAD_AXIS_H
#define CGSS_GAMEPAD_AXIS_H

// Backend-agnostic gamepad axis enum. Numeric values are ABI (exposed to
// scripts). Positions are always normalized to [-1.0, 1.0]: backends with a
// different native range rescale in their Ops.

namespace cgss {

	enum class GamepadAxis : int {
		LeftX        = 0,
		LeftY        = 1,
		RightX       = 2,
		RightY       = 3,
		LeftTrigger  = 4, // analog [0.0, 1.0]
		RightTrigger = 5, // analog [0.0, 1.0]
	};

	#define CGSS_GAMEPAD_AXIS_LIST(X) \
		X(LeftX) X(LeftY) X(RightX) X(RightY) \
		X(LeftTrigger) X(RightTrigger)

}

#endif
