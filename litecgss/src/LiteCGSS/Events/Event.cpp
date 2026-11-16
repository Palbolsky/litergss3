#include "LiteCGSS/Events/Event.h"

#include "LiteCGSS/Backend/ActiveBackend.h"

template <class B>
cgss::basic_Event<B>::basic_Event() :
	m_event(std::make_unique<typename B::native_event>()) {}

template <class B>
cgss::basic_Event<B>::basic_Event(const basic_Event& other) :
	m_event(std::make_unique<typename B::native_event>(*other.m_event)) {}

template <class B>
cgss::basic_Event<B>::basic_Event(basic_Event&& other) noexcept = default;

template <class B>
cgss::basic_Event<B>& cgss::basic_Event<B>::operator=(const basic_Event& other) {
	*m_event = *other.m_event;
	return *this;
}

template <class B>
cgss::basic_Event<B>& cgss::basic_Event<B>::operator=(basic_Event&& other) noexcept = default;

template <class B>
cgss::basic_Event<B>::~basic_Event() = default;

template <class B>
typename cgss::basic_Event<B>::native_type& cgss::basic_Event<B>::native() noexcept { return *m_event; }

template <class B>
const typename cgss::basic_Event<B>::native_type& cgss::basic_Event<B>::native() const noexcept { return *m_event; }

// ---- Typed accessor bodies ----
// Each forwards to the active backend's Ops thunk (lazy translation —
// see the rationale in Event.h). SFMLBackend::Ops returns real payloads
// from the backing sf::Event; RaylibBackend::Ops currently returns
// nullopt/Closed until PR A2 brings its synthetic event queue online.

template <class B>
cgss::EventType cgss::basic_Event<B>::type() const noexcept {
	return B::Ops::event_type(*m_event);
}

template <class B>
std::optional<cgss::KeyEvent> cgss::basic_Event<B>::asKeyPress() const {
	return B::Ops::event_as_key_press(*m_event);
}

template <class B>
std::optional<cgss::KeyEvent> cgss::basic_Event<B>::asKeyRelease() const {
	return B::Ops::event_as_key_release(*m_event);
}

template <class B>
std::optional<cgss::TextEvent> cgss::basic_Event<B>::asTextEntered() const {
	return B::Ops::event_as_text_entered(*m_event);
}

template <class B>
std::optional<cgss::MouseButtonEvent> cgss::basic_Event<B>::asMouseButtonPress() const {
	return B::Ops::event_as_mouse_button_press(*m_event);
}

template <class B>
std::optional<cgss::MouseButtonEvent> cgss::basic_Event<B>::asMouseButtonRelease() const {
	return B::Ops::event_as_mouse_button_release(*m_event);
}

template <class B>
std::optional<cgss::MouseMoveEvent> cgss::basic_Event<B>::asMouseMove() const {
	return B::Ops::event_as_mouse_move(*m_event);
}

template <class B>
std::optional<cgss::MouseWheelEvent> cgss::basic_Event<B>::asMouseWheel() const {
	return B::Ops::event_as_mouse_wheel(*m_event);
}

template <class B>
std::optional<cgss::SizeEvent> cgss::basic_Event<B>::asResize() const {
	return B::Ops::event_as_resize(*m_event);
}

template <class B>
std::optional<cgss::GamepadButtonEvent> cgss::basic_Event<B>::asGamepadButtonPress() const {
	return B::Ops::event_as_gamepad_button_press(*m_event);
}

template <class B>
std::optional<cgss::GamepadButtonEvent> cgss::basic_Event<B>::asGamepadButtonRelease() const {
	return B::Ops::event_as_gamepad_button_release(*m_event);
}

template <class B>
std::optional<cgss::GamepadMoveEvent> cgss::basic_Event<B>::asGamepadMove() const {
	return B::Ops::event_as_gamepad_move(*m_event);
}

template <class B>
std::optional<cgss::GamepadConnectEvent> cgss::basic_Event<B>::asGamepadConnect() const {
	return B::Ops::event_as_gamepad_connect(*m_event);
}

template <class B>
std::optional<cgss::GamepadConnectEvent> cgss::basic_Event<B>::asGamepadDisconnect() const {
	return B::Ops::event_as_gamepad_disconnect(*m_event);
}

template <class B>
std::optional<cgss::TouchEvent> cgss::basic_Event<B>::asTouch() const {
	return B::Ops::event_as_touch(*m_event);
}

template <class B>
std::optional<cgss::SensorEvent> cgss::basic_Event<B>::asSensor() const {
	return B::Ops::event_as_sensor(*m_event);
}

template class cgss::basic_Event<cgss::backend::ActiveBackend>;
