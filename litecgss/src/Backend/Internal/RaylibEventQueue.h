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

	// Current-view tracking. sf::View's "set it on the target and leave it"
	// semantic has no raylib analogue (Camera2D is scoped by
	// BeginMode2D/EndMode2D; scissor by BeginScissorMode/EndScissorMode).
	// Instead, target_set_view stashes the view here and the draw ops read
	// it to compute per-call screen-space positions + manage the scissor
	// state for sub-viewport clipping. Single-threaded by design, matching
	// raylib's own rendering model.
	void                     setCurrentView(const cgss::backend::raylib::View& v) noexcept;
	const cgss::backend::raylib::View& currentView() noexcept;
	// Drop any active scissor started by setCurrentView. Called from
	// window_display before EndDrawing to leave GL state tidy across frames.
	void                     endCurrentScissor() noexcept;

	// Track the active draw target's pixel dimensions. raylib's GetScreenWidth
	// /Height always reports the window size; when a draw is bracketed in
	// BeginTextureMode the actual target is an FBO of different dimensions,
	// and view-zoom + scissor math must use those FBO dims, not the window's.
	// target_set_view writes these from the bound `native_render_target`
	// (`t.texture.width/height` for an FBO, GetScreenWidth/Height for the
	// window placeholder); derive_current_view + setCurrentView read them.
	// Single-threaded by design, like every other piece of state in this TU.
	void                     setCurrentTargetSize(float width, float height) noexcept;
	float                    currentTargetWidth() noexcept;
	float                    currentTargetHeight() noexcept;

	// Frame-boundary signal. Called from window_display so the next frame's
	// event-queue drain can re-enter refillQueue once (see g_frameRefillDone
	// in the .cpp for the rationale — GetMouseDelta and friends are
	// frame-level state that stays constant until raylib's PollInputEvents,
	// so we must not re-poll them within a single frame).
	void                     markFrameBoundary() noexcept;

}

#endif
