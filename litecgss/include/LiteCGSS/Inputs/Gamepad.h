#ifndef CGSS_INPUTS_GAMEPAD_H
#define CGSS_INPUTS_GAMEPAD_H

// Polled gamepad state. Backend-neutral (see Inputs/Keyboard.h).

#include "LiteCGSS/Events/GamepadAxis.h"
#include "LiteCGSS/Events/GamepadButton.h"

namespace cgss::gamepad {

	bool isConnected(unsigned int id);
	bool isButtonPressed(unsigned int id, GamepadButton button);
	bool isButtonReleased(unsigned int id, GamepadButton button);

	unsigned int axisCount(unsigned int id);
	// Normalized to [-1, 1].
	float axisPosition(unsigned int id, GamepadAxis axis);

	// Never null; empty when the backend does not know the name.
	const char* name(unsigned int id);

	// No-op on backends without rumble support.
	void setVibration(unsigned int id, float leftMotor, float rightMotor, float durationMs);

	// Refresh the polled state outside of the window event loop.
	void update();

}

#endif
