#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_KEY_TRANSLATION_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_KEY_TRANSLATION_H

// Backend-internal: cgss::KeyCode <-> raylib KEY_* translation.
// Raylib's key enum uses sparse GLFW-style codepoints (A=65, F1=290, etc.)
// while cgss::KeyCode is dense SFML 2.6-layout (A=0, F1=85, ...). These
// helpers bridge the two. Used by RaylibBackend::Ops and the event queue.

#include "LiteCGSS/Events/KeyCode.h"
#include "LiteCGSS/Events/Scancode.h"

namespace cgss::backend::raylib_detail {

	// Map a cgss::KeyCode to its raylib KEY_* integer. Returns 0 (KEY_NULL)
	// for cgss values that have no raylib equivalent (e.g. F13/F14/F15 —
	// raylib only ships F1..F12).
	int toRaylibKey(cgss::KeyCode code) noexcept;

	// Reverse — a raylib KEY_* integer back to cgss::KeyCode. Returns
	// cgss::KeyCode::Unknown for unrecognized values (including raylib's
	// Android-specific keys).
	cgss::KeyCode fromRaylibKey(int raylibKey) noexcept;

	// raylib's KEY_* are GLFW key tokens: physical positions named after the
	// US layout — scancodes, in cgss terms. KEY_NULL / Unknown when unmapped.
	cgss::Scancode scancodeFromRaylibKey(int raylibKey) noexcept;
	int raylibKeyFromScancode(cgss::Scancode scancode) noexcept;

}

#endif
