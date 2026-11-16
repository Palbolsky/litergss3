#include "LiteCGSS/Inputs/Keyboard.h"

#include "Backend/ActiveBackend.h"

using Ops = cgss::backend::ActiveBackend::Ops;

bool cgss::keyboard::isKeyPressed(KeyCode key) {
	return Ops::keyboard_is_key_pressed(key);
}
