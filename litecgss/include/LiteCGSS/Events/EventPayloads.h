#ifndef CGSS_EVENT_PAYLOADS_H
#define CGSS_EVENT_PAYLOADS_H

// Plain-aggregate payload structs returned by `cgss::Event::as*()` methods.
// Backend-agnostic POD — no backend type leaks into the public API.
//
// Where cgss adds a field (horizontal wheel, touch phase), the new field is
// appended and documented.

#include <cstdint>

#include "LiteCGSS/Events/KeyCode.h"
#include "LiteCGSS/Events/Scancode.h"
#include "LiteCGSS/Events/MouseButton.h"
#include "LiteCGSS/Events/GamepadButton.h"
#include "LiteCGSS/Events/GamepadAxis.h"
#include "LiteCGSS/Events/TouchPhase.h"
#include "LiteCGSS/Events/SensorType.h"

namespace cgss {

	struct KeyEvent {
		KeyCode  code     = KeyCode::Unknown;
		bool     alt      = false;
		bool     control  = false;
		bool     shift    = false;
		bool     system   = false;
		Scancode scancode = Scancode::Unknown; // physical key (appended)
	};

	struct TextEvent {
		std::uint32_t unicode = 0u;
	};

	struct SizeEvent {
		unsigned int width  = 0u;
		unsigned int height = 0u;
	};

	struct MouseMoveEvent {
		int x = 0;
		int y = 0;
	};

	struct MouseButtonEvent {
		MouseButton button = MouseButton::Left;
		int         x      = 0;
		int         y      = 0;
	};

	struct MouseWheelEvent {
		float delta      = 0.f;
		int   x          = 0;
		int   y          = 0;
		bool  horizontal = false; // true for HorizontalWheel, false for VerticalWheel
	};

	struct GamepadButtonEvent {
		unsigned int  id     = 0u;
		GamepadButton button = GamepadButton::Unknown;
	};

	struct GamepadMoveEvent {
		unsigned int id       = 0u;
		GamepadAxis  axis     = GamepadAxis::LeftX;
		float        position = 0.f; // normalized [-1.0, 1.0] (triggers: [0.0, 1.0])
	};

	struct GamepadConnectEvent {
		unsigned int id = 0u;
	};

	struct TouchEvent {
		unsigned int finger = 0u;
		int          x      = 0;
		int          y      = 0;
		TouchPhase   phase  = TouchPhase::Began;
	};

	struct SensorEvent {
		SensorType type = SensorType::Accelerometer;
		float      x    = 0.f;
		float      y    = 0.f;
		float      z    = 0.f;
	};

}

#endif
