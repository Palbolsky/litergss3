#ifndef SfJoystick_H
#define SfJoystick_H

// LiteRGSS2's joystick numbering, used by Sf::Joystick and the joystick
// callbacks of DisplayWindow: the raw ids SFML reports for an XInput-style
// pad (what PSDK's key bindings and AXIS_MAPPING are written against).
//   buttons: 0 A, 1 B, 2 X, 3 Y, 4 LB, 5 RB, 6 Back, 7 Start, 8 LStick,
//            9 RStick, 10 Guide; 12-15 D-pad up / down / left / right, for
//            pads reporting the D-pad as buttons.
//   axes:    0 X, 1 Y (left stick), 2 Z (left trigger), 3 R (right trigger),
//            4 U, 5 V (right stick), 6 PovX, 7 PovY (D-pad); positions in
//            [-100, 100].

#include <optional>

#include "LiteCGSS/Events/GamepadAxis.h"
#include "LiteCGSS/Events/GamepadButton.h"

namespace legacy_joystick {

	constexpr unsigned int kCount       = 8;
	constexpr unsigned int kButtonCount = 32;
	constexpr unsigned int kAxisCount   = 8;

	// -1 when the button has no LiteRGSS2 index.
	int buttonIndex(cgss::GamepadButton button);
	std::optional<cgss::GamepadButton> button(long index);

	long axisId(cgss::GamepadAxis axis);
	std::optional<cgss::GamepadAxis> axis(long id);

	// cgss positions are normalized; LiteRGSS2's are SFML's [-100, 100].
	inline double position(float normalized) { return static_cast<double>(normalized) * 100.0; }

}

void Init_SfJoystick();

#endif
