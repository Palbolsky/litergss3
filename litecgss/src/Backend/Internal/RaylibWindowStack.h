#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_WINDOW_STACK_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_WINDOW_STACK_H

// raylib opens one OS window. The cgss windows are stacked on it: the last
// one opened owns the screen (its size, title, draws and events); when it
// closes, the one below gets it back. PSDK's error screen, a second
// DisplayWindow opened over the game, takes the screen this way.

#include "LiteCGSS/Backend/RaylibBackendFwd.h"

namespace cgss::backend::raylib_detail {

	// Opened (or reopened): on top, owning the screen.
	void windowOpened(raylib::Window& window);
	// Closed: off the stack. Returns the window owning the screen now
	// (nullptr: none left).
	raylib::Window* windowClosed(raylib::Window& window);
	// nullptr when no window is open.
	raylib::Window* screenOwner();

}

#endif
