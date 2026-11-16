#ifndef CGSS_GAMEPAD_BUTTON_H
#define CGSS_GAMEPAD_BUTTON_H

// Backend-agnostic gamepad button enum, named after the physical position
// of each button (face / trigger / middle / thumb). Numeric values are ABI
// (exposed to scripts). Backends whose native API only knows raw button
// indices translate through a lookup table in their Ops.

namespace cgss {

	enum class GamepadButton : int {
		Unknown            = 0,

		// Left d-pad / face-buttons-left cluster
		LeftFaceUp         = 1,
		LeftFaceRight      = 2,
		LeftFaceDown       = 3,
		LeftFaceLeft       = 4,

		// Right face cluster (A/B/X/Y, △/○/×/□)
		RightFaceUp        = 5,
		RightFaceRight     = 6,
		RightFaceDown      = 7,
		RightFaceLeft      = 8,

		// Shoulder buttons / triggers (digital)
		LeftTrigger1       = 9,
		LeftTrigger2       = 10,
		RightTrigger1      = 11,
		RightTrigger2      = 12,

		// Center cluster (Back/Start/Guide)
		MiddleLeft         = 13,
		Middle             = 14,
		MiddleRight        = 15,

		// Thumbstick clicks
		LeftThumb          = 16,
		RightThumb         = 17,
	};

	#define CGSS_GAMEPAD_BUTTON_LIST(X) \
		X(Unknown) \
		X(LeftFaceUp) X(LeftFaceRight) X(LeftFaceDown) X(LeftFaceLeft) \
		X(RightFaceUp) X(RightFaceRight) X(RightFaceDown) X(RightFaceLeft) \
		X(LeftTrigger1) X(LeftTrigger2) X(RightTrigger1) X(RightTrigger2) \
		X(MiddleLeft) X(Middle) X(MiddleRight) \
		X(LeftThumb) X(RightThumb)

}

#endif
