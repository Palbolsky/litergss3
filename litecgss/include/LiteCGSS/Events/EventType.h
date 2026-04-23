#ifndef CGSS_EVENT_TYPE_H
#define CGSS_EVENT_TYPE_H

// Backend-agnostic event type tag. Layout mirrors SFML 2.6
// sf::Event::EventType so that SFMLBackend::Ops::event_type is a
// `static_cast`. Raylib backend's synthetic event queue stores the
// cgss::EventType directly on its native_event POD.

namespace cgss {

	enum class EventType : int {
		Closed                  = 0,
		Resized                 = 1,
		LostFocus               = 2,
		GainedFocus             = 3,
		TextEntered             = 4,
		KeyPressed              = 5,
		KeyReleased             = 6,
		MouseWheelScrolled      = 8, // 7 is deprecated sf::Event::MouseWheelMoved
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
