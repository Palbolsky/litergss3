#ifndef CGSS_INPUTS_INJECTION_H
#define CGSS_INPUTS_INJECTION_H

// Synthetic input, for development tools (an agent's `press`): an injected key
// reaches the game the way a real keyboard's does — a KeyPressed / KeyReleased
// event from the next DisplayWindow::popEvent (before the device's own), and
// keyboard::isKeyPressed / isScancodePressed answering true while it is down.
// Backend-neutral; process-global; the caller's thread is the event pump's.

#include "LiteCGSS/Events/KeyCode.h"
#include "LiteCGSS/Events/Scancode.h"

namespace cgss::input {

	// Presses (down) or releases a key. scancode Unknown: derived from the
	// key with the current layout (keyboard::delocalize).
	void injectKey(KeyCode key, Scancode scancode, bool down);

	// Releases every injected key still down (with their KeyReleased events).
	void releaseInjectedKeys();

	bool isInjectedKeyDown(KeyCode key);
	bool isInjectedScancodeDown(Scancode scancode);

}

#endif
