#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_EVENT_QUEUE_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_EVENT_QUEUE_H

// Backend-internal: synthesizes a queue of cgss-typed events from raylib's
// polled input API (IsKeyDown/GetKeyPressed/IsMouseButtonDown/etc.).
// Raylib has no event pump; `RaylibBackend::Ops::window_poll_event` drains
// this queue, calling `refillQueue()` when empty to diff the current frame
// against the previous snapshot.
//
// All state is process-global and NOT thread-safe. Callers must invoke
// `popEvent` from the main thread only (raylib has the same restriction).

#include "LiteCGSS/Backend/RaylibBackendFwd.h"

namespace cgss::backend::raylib_detail {

	// Pull one event off the queue. Returns false if empty.
	bool tryPopEvent(cgss::backend::raylib::Event& out) noexcept;

	// Diff this frame vs. last against raylib's polled input state and push
	// the resulting events onto the queue. Called automatically by
	// `tryPopEvent` when the queue is empty, so most consumers can ignore
	// this. Exposed for tests that want to force a refill.
	void refillQueue();

	// Window close latch — shared between RaylibBackend::Ops::window_close
	// (sets it explicitly) and window_is_open (checks it). Needed because
	// raylib's WindowShouldClose() is a transient query tied to OS input
	// state, not a sticky "was closed" flag like sf::Window::isOpen().
	void markWindowClosed() noexcept;
	bool isWindowClosed() noexcept;
	void resetWindowClosed() noexcept;

}

#endif
