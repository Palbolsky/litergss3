#include "LiteCGSS/Inputs/Keyboard.h"
#include "LiteCGSS/Inputs/Injection.h"

#include "Backend/ActiveBackend.h"

using Ops = cgss::backend::ActiveBackend::Ops;

bool cgss::keyboard::isKeyPressed(KeyCode key) {
	return Ops::keyboard_is_key_pressed(key) || input::isInjectedKeyDown(key);
}

bool cgss::keyboard::isScancodePressed(Scancode scancode) {
	return Ops::keyboard_is_scancode_pressed(scancode) || input::isInjectedScancodeDown(scancode);
}

cgss::KeyCode cgss::keyboard::localize(Scancode scancode) {
	return Ops::keyboard_localize(scancode);
}

cgss::Scancode cgss::keyboard::delocalize(KeyCode key) {
	return Ops::keyboard_delocalize(key);
}
