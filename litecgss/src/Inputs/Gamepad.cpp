#include "LiteCGSS/Inputs/Gamepad.h"

#include "Backend/ActiveBackend.h"

using Ops = cgss::backend::ActiveBackend::Ops;

bool cgss::gamepad::isConnected(unsigned int id) {
	return Ops::gamepad_is_connected(id);
}

bool cgss::gamepad::isButtonPressed(unsigned int id, GamepadButton button) {
	return Ops::gamepad_is_button_pressed(id, button);
}

bool cgss::gamepad::isButtonReleased(unsigned int id, GamepadButton button) {
	return Ops::gamepad_is_button_released(id, button);
}

unsigned int cgss::gamepad::axisCount(unsigned int id) {
	return Ops::gamepad_axis_count(id);
}

float cgss::gamepad::axisPosition(unsigned int id, GamepadAxis axis) {
	return Ops::gamepad_axis_position(id, axis);
}

const char* cgss::gamepad::name(unsigned int id) {
	const char* name = Ops::gamepad_name(id);
	return name != nullptr ? name : "";
}

void cgss::gamepad::setVibration(unsigned int id, float leftMotor, float rightMotor, float durationMs) {
	Ops::gamepad_set_vibration(id, leftMotor, rightMotor, durationMs);
}

void cgss::gamepad::update() {
	Ops::gamepad_update();
}
