#include "LiteCGSS/Inputs/Mouse.h"

#include "Backend/ActiveBackend.h"

using Backend = cgss::backend::ActiveBackend;
using Ops     = Backend::Ops;

bool cgss::mouse::isButtonPressed(MouseButton button) {
	return Ops::mouse_is_button_pressed(button);
}

cgss::backend::Point2i cgss::mouse::position() {
	return Ops::mouse_get_position();
}

void cgss::mouse::setPosition(int x, int y) {
	Ops::mouse_set_position(x, y);
}

float cgss::mouse::wheelDelta() {
	if constexpr (Backend::has_wheel_state_query) {
		return Ops::mouse_wheel_delta();
	} else {
		return 0.f;
	}
}

bool cgss::mouse::hasWheelStateQuery() noexcept {
	return Backend::has_wheel_state_query;
}
