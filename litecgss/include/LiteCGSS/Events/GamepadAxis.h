#ifndef CGSS_GAMEPAD_AXIS_H
#define CGSS_GAMEPAD_AXIS_H

// Backend-agnostic gamepad axis enum. Naming mirrors raylib's GAMEPAD_AXIS_*
// (ext/LiteRGSS/inputs/Gamepad.cpp already exposes these names to Ruby).
// SFMLBackend::Ops translates to sf::Joystick::Axis X/Y/Z/R/U/V indices
// in gamepad_axis_position; normalization to [-1.0, 1.0] follows raylib's
// convention (SFML returns [-100, 100] which the Op divides by 100).

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
