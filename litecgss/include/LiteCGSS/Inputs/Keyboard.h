#ifndef CGSS_INPUTS_KEYBOARD_H
#define CGSS_INPUTS_KEYBOARD_H

// Polled keyboard state. Backend-neutral: implemented over the active
// backend's Ops in src/Inputs/, so including this header does not pull any
// backend header into the consumer.

#include "LiteCGSS/Events/KeyCode.h"
#include "LiteCGSS/Events/Scancode.h"

namespace cgss::keyboard {

	bool isKeyPressed(KeyCode key);
	bool isScancodePressed(Scancode scancode);

	// Layout conversions: the key a physical key produces with the current
	// keyboard layout, and back. Unknown when there is none.
	KeyCode localize(Scancode scancode);
	Scancode delocalize(KeyCode key);

}

#endif
