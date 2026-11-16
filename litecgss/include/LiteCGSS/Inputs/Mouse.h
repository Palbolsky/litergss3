#ifndef CGSS_INPUTS_MOUSE_H
#define CGSS_INPUTS_MOUSE_H

// Polled mouse state. Backend-neutral (see Inputs/Keyboard.h).

#include "LiteCGSS/Backend/BackendCommon.h"
#include "LiteCGSS/Events/MouseButton.h"

namespace cgss::mouse {

	bool isButtonPressed(MouseButton button);

	// Desktop coordinates.
	backend::Point2i position();
	void setPosition(int x, int y);

	// Wheel movement since the last frame. Backends that only report the
	// wheel through events (hasWheelStateQuery() == false, e.g. SFML)
	// always return 0: poll MouseWheel events instead.
	float wheelDelta();
	bool hasWheelStateQuery() noexcept;

}

#endif
