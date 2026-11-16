#ifndef CGSS_EVENT_TYPE_H
#define CGSS_EVENT_TYPE_H

// Backend-agnostic event type tag. Each backend translates its native
// events to it in Ops::event_type. Numeric values must stay stable: value 7
// is intentionally unused (a retired wheel event).

namespace cgss {

	enum class EventType : int {
		Closed                  = 0,
		Resized                 = 1,
		LostFocus               = 2,
		GainedFocus             = 3,
		TextEntered             = 4,
		KeyPressed              = 5,
		KeyReleased             = 6,
		MouseWheelScrolled      = 8, // 7 is retired (see header comment)
		MouseButtonPressed      = 9,
		MouseButtonReleased     = 10,
		MouseMoved              = 11,
		MouseEntered            = 12,
		MouseLeft               = 13,
		JoystickButtonPressed   = 14,
		JoystickButtonReleased  = 15,
		JoystickMoved           = 16,
		JoystickConnected       = 17,
		JoystickDisconnected    = 18,
		TouchBegan              = 19,
		TouchMoved              = 20,
		TouchEnded              = 21,
		SensorChanged           = 22,
	};

}

#endif
