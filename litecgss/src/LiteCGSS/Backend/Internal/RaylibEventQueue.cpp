#include "LiteCGSS/Backend/Internal/RaylibEventQueue.h"
#include "LiteCGSS/Backend/Internal/RaylibKeyTranslation.h"

#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Events/KeyCode.h"
#include "LiteCGSS/Events/MouseButton.h"

#include <raylib.h>

#include <array>
#include <cstring>
#include <deque>

namespace cgss::backend::raylib_detail {

	namespace {
		// All raylib gamepad buttons (GAMEPAD_BUTTON_*) fit in [0..17] and
		// map 1:1 to cgss::GamepadButton, so we index by raylib int.
		constexpr unsigned int kMaxGamepads     = 4;
		constexpr unsigned int kGamepadButtons  = 18;  // GAMEPAD_BUTTON_UNKNOWN .. RIGHT_THUMB
		constexpr unsigned int kGamepadAxes     = 6;
		constexpr unsigned int kMouseButtons    = 8;   // MOUSE_BUTTON_LEFT..BACK + slack
		constexpr unsigned int kMaxRaylibKey    = 512; // raylib max KEY_* is ~348
		constexpr unsigned int kMaxTouches      = 16;
		constexpr float        kAxisEpsilon     = 0.01f;

		struct PrevFrameState {
			bool  initialized = false;
			bool  closedLatched = false;
			bool  windowFocused = true;

			std::array<bool, kMaxRaylibKey> keyDown{};
			std::array<bool, kMouseButtons> mouseButtonDown{};

			std::array<bool, kMaxGamepads>                                   gamepadConnected{};
			std::array<std::array<bool,  kGamepadButtons>, kMaxGamepads>     gamepadButtonDown{};
			std::array<std::array<float, kGamepadAxes>,    kMaxGamepads>     gamepadAxis{};

			std::array<int, kMaxTouches> touchIds{};
			unsigned int touchCount = 0u;
		};

		PrevFrameState                        g_prev{};
		std::deque<cgss::backend::raylib::Event> g_queue{};

		void pushEvent(const cgss::backend::raylib::Event& ev) {
			g_queue.push_back(ev);
		}

		// Convenience: build a default-constructed Event, set type + one
		// payload field, push. The other fields stay zero-initialized —
		// consumers switch on type() so the inactive fields are never read.
		cgss::backend::raylib::Event makeEvent(cgss::EventType type) {
			cgss::backend::raylib::Event e;
			e.type = type;
			return e;
		}

		void pollKeyboard() {
			// Modifier state for KeyEvent payload. Checked once per refill.
			const bool alt     = IsKeyDown(KEY_LEFT_ALT)     || IsKeyDown(KEY_RIGHT_ALT);
			const bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
			const bool shift   = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);
			const bool sys     = IsKeyDown(KEY_LEFT_SUPER)   || IsKeyDown(KEY_RIGHT_SUPER);

			// Key presses: drain GetKeyPressed queue (returns 0 when empty).
			while (int k = GetKeyPressed()) {
				auto ev = makeEvent(cgss::EventType::KeyPressed);
				ev.key = cgss::KeyEvent{ fromRaylibKey(k), alt, control, shift, sys };
				pushEvent(ev);
				if (k >= 0 && static_cast<unsigned int>(k) < kMaxRaylibKey) {
					g_prev.keyDown[k] = true;
				}
			}

			// Key releases: diff previous-frame down-state against current.
			// IsKeyDown is cheap but iterating 0..512 is wasteful; iterate
			// only the bits we already know were down.
			for (unsigned int k = 0; k < kMaxRaylibKey; ++k) {
				if (g_prev.keyDown[k] && !IsKeyDown(static_cast<int>(k))) {
					auto ev = makeEvent(cgss::EventType::KeyReleased);
					ev.key = cgss::KeyEvent{ fromRaylibKey(static_cast<int>(k)), alt, control, shift, sys };
					pushEvent(ev);
					g_prev.keyDown[k] = false;
				}
			}

			// Text input: drain GetCharPressed.
			while (int c = GetCharPressed()) {
				auto ev = makeEvent(cgss::EventType::TextEntered);
				ev.text = cgss::TextEvent{ static_cast<std::uint32_t>(c) };
				pushEvent(ev);
			}
		}

		void pollMouse() {
			const int mx = GetMouseX();
			const int my = GetMouseY();

			for (unsigned int i = 0; i < kMouseButtons; ++i) {
				const bool nowDown = IsMouseButtonDown(static_cast<int>(i));
				if (nowDown && !g_prev.mouseButtonDown[i]) {
					auto ev = makeEvent(cgss::EventType::MouseButtonPressed);
					ev.mouseButton = cgss::MouseButtonEvent{ static_cast<cgss::MouseButton>(i), mx, my };
					pushEvent(ev);
				} else if (!nowDown && g_prev.mouseButtonDown[i]) {
					auto ev = makeEvent(cgss::EventType::MouseButtonReleased);
					ev.mouseButton = cgss::MouseButtonEvent{ static_cast<cgss::MouseButton>(i), mx, my };
					pushEvent(ev);
				}
				g_prev.mouseButtonDown[i] = nowDown;
			}

			const auto delta = GetMouseDelta();
			if (delta.x != 0.f || delta.y != 0.f) {
				auto ev = makeEvent(cgss::EventType::MouseMoved);
				ev.mouseMove = cgss::MouseMoveEvent{ mx, my };
				pushEvent(ev);
			}

			const auto wheel = GetMouseWheelMoveV();
			if (wheel.y != 0.f) {
				auto ev = makeEvent(cgss::EventType::MouseWheelScrolled);
				ev.mouseWheel = cgss::MouseWheelEvent{ wheel.y, mx, my, false };
				pushEvent(ev);
			}
			if (wheel.x != 0.f) {
				auto ev = makeEvent(cgss::EventType::MouseWheelScrolled);
				ev.mouseWheel = cgss::MouseWheelEvent{ wheel.x, mx, my, true };
				pushEvent(ev);
			}
		}

		void pollGamepads() {
			for (unsigned int id = 0; id < kMaxGamepads; ++id) {
				const bool nowConnected = IsGamepadAvailable(static_cast<int>(id));

				if (nowConnected != g_prev.gamepadConnected[id]) {
					auto ev = makeEvent(nowConnected
						? cgss::EventType::JoystickConnected
						: cgss::EventType::JoystickDisconnected);
					ev.gamepadConnect = cgss::GamepadConnectEvent{ id };
					pushEvent(ev);
					g_prev.gamepadConnected[id] = nowConnected;
					// On a fresh connect, zero per-gamepad state so first-frame
					// axis/button values don't spuriously trigger deltas.
					if (nowConnected) {
						g_prev.gamepadButtonDown[id].fill(false);
						g_prev.gamepadAxis[id].fill(0.f);
					}
				}
				if (!nowConnected) continue;

				// Buttons — cgss::GamepadButton numeric layout matches
				// raylib's GAMEPAD_BUTTON_* 1:1.
				for (unsigned int b = 0; b < kGamepadButtons; ++b) {
					const bool nowDown = IsGamepadButtonDown(static_cast<int>(id), static_cast<int>(b));
					if (nowDown && !g_prev.gamepadButtonDown[id][b]) {
						auto ev = makeEvent(cgss::EventType::JoystickButtonPressed);
						ev.gamepadButton = cgss::GamepadButtonEvent{ id, static_cast<cgss::GamepadButton>(b) };
						pushEvent(ev);
					} else if (!nowDown && g_prev.gamepadButtonDown[id][b]) {
						auto ev = makeEvent(cgss::EventType::JoystickButtonReleased);
						ev.gamepadButton = cgss::GamepadButtonEvent{ id, static_cast<cgss::GamepadButton>(b) };
						pushEvent(ev);
					}
					g_prev.gamepadButtonDown[id][b] = nowDown;
				}

				// Axes — 1:1 numeric layout with cgss::GamepadAxis.
				for (unsigned int a = 0; a < kGamepadAxes; ++a) {
					const float now = GetGamepadAxisMovement(static_cast<int>(id), static_cast<int>(a));
					const float prev = g_prev.gamepadAxis[id][a];
					const float diff = now - prev;
					if ((diff >= 0.f ? diff : -diff) >= kAxisEpsilon) {
						auto ev = makeEvent(cgss::EventType::JoystickMoved);
						ev.gamepadMove = cgss::GamepadMoveEvent{ id, static_cast<cgss::GamepadAxis>(a), now };
						pushEvent(ev);
						g_prev.gamepadAxis[id][a] = now;
					}
				}
			}
		}

		void pollTouches() {
			const unsigned int nowCount = static_cast<unsigned int>(GetTouchPointCount());
			std::array<int, kMaxTouches> nowIds{};
			const unsigned int usableCount = nowCount > kMaxTouches ? kMaxTouches : nowCount;
			for (unsigned int i = 0; i < usableCount; ++i) {
				nowIds[i] = GetTouchPointId(static_cast<int>(i));
			}

			// Ended = ids in prev but not in now
			for (unsigned int i = 0; i < g_prev.touchCount; ++i) {
				const int id = g_prev.touchIds[i];
				bool stillThere = false;
				for (unsigned int j = 0; j < usableCount; ++j) {
					if (nowIds[j] == id) { stillThere = true; break; }
				}
				if (!stillThere) {
					auto ev = makeEvent(cgss::EventType::TouchEnded);
					ev.touch = cgss::TouchEvent{ static_cast<unsigned int>(id), 0, 0, cgss::TouchPhase::Ended };
					pushEvent(ev);
				}
			}

			// Began = ids in now but not in prev; Moved = ids in both.
			for (unsigned int i = 0; i < usableCount; ++i) {
				const int id = nowIds[i];
				bool wasThere = false;
				for (unsigned int j = 0; j < g_prev.touchCount; ++j) {
					if (g_prev.touchIds[j] == id) { wasThere = true; break; }
				}
				const auto pos = GetTouchPosition(static_cast<int>(i));
				auto ev = makeEvent(wasThere ? cgss::EventType::TouchMoved : cgss::EventType::TouchBegan);
				ev.touch = cgss::TouchEvent{
					static_cast<unsigned int>(id),
					static_cast<int>(pos.x),
					static_cast<int>(pos.y),
					wasThere ? cgss::TouchPhase::Moved : cgss::TouchPhase::Began
				};
				pushEvent(ev);
			}

			g_prev.touchIds = nowIds;
			g_prev.touchCount = usableCount;
		}

		void pollWindow() {
			// Resize: raylib's IsWindowResized() is a one-shot that's only
			// true for the frame after the resize happens.
			if (IsWindowResized()) {
				auto ev = makeEvent(cgss::EventType::Resized);
				ev.size = cgss::SizeEvent{
					static_cast<unsigned int>(GetScreenWidth()),
					static_cast<unsigned int>(GetScreenHeight())
				};
				pushEvent(ev);
			}

			// Focus — edge-diff. raylib exposes IsWindowFocused() as the
			// current state.
			const bool nowFocused = IsWindowFocused();
			if (nowFocused != g_prev.windowFocused) {
				auto ev = makeEvent(nowFocused
					? cgss::EventType::GainedFocus
					: cgss::EventType::LostFocus);
				pushEvent(ev);
				g_prev.windowFocused = nowFocused;
			}

			// Close — latch once so we don't spam Closed events while the
			// user-code hasn't torn down the window yet.
			if (!g_prev.closedLatched && WindowShouldClose()) {
				pushEvent(makeEvent(cgss::EventType::Closed));
				g_prev.closedLatched = true;
			}
		}
	}

	void refillQueue() {
		// First call: sync prev-state to current so the first frame doesn't
		// generate spurious events (e.g. "keys went from false to true"
		// during startup, or "connected gamepads became connected").
		if (!g_prev.initialized) {
			for (unsigned int id = 0; id < kMaxGamepads; ++id) {
				g_prev.gamepadConnected[id] = IsGamepadAvailable(static_cast<int>(id));
			}
			g_prev.windowFocused = IsWindowFocused();
			g_prev.initialized = true;
		}

		pollWindow();
		pollKeyboard();
		pollMouse();
		pollGamepads();
		pollTouches();
	}

	bool tryPopEvent(cgss::backend::raylib::Event& out) noexcept {
		if (g_queue.empty()) {
			refillQueue();
		}
		if (g_queue.empty()) return false;
		out = g_queue.front();
		g_queue.pop_front();
		return true;
	}

}
