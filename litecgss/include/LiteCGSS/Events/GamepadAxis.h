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
		// D-pad as axes, in {-1, 0, 1}: right / up are positive. Backends
		// reporting the D-pad as buttons synthesize these from them.
		DpadX        = 6,
		DpadY        = 7,
	};

	#define CGSS_GAMEPAD_AXIS_LIST(X) \
		X(LeftX) X(LeftY) X(RightX) X(RightY) \
		X(LeftTrigger) X(RightTrigger) \
		X(DpadX) X(DpadY)

}

#endif
