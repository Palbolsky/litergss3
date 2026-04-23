#ifndef CGSS_EVENT_H
#define CGSS_EVENT_H

#include <memory>
#include <optional>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Events/EventFwd.h"
#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Events/EventPayloads.h"

namespace cgss {

	// Templated wrapper over `Backend::native_event`. Consumers refer to it
	// as `cgss::Event`. The native event handle is reachable only via
	// `Backend::Ops::native_of(event)` from impl-side .cpps that have the
	// full backend header included; consumers cannot escape the abstraction.
	//
	// Typed public API: `type()` returns the backend-neutral cgss::EventType;
	// `as*()` returns a typed std::optional<Payload>. Bodies forward to
	// `ActiveBackend::Ops::event_*` thunks that know how to translate from
	// the native event representation — see per-backend Ops structs.
	template <class B>
	class basic_Event {
		friend struct cgss::backend::ActiveBackend::Ops;
	public:
		using native_type = typename B::native_event;

		basic_Event();
		basic_Event(const basic_Event& other);
		basic_Event(basic_Event&& other) noexcept;
		basic_Event& operator=(const basic_Event& other);
		basic_Event& operator=(basic_Event&& other) noexcept;
		~basic_Event();

		EventType type() const noexcept;

		std::optional<KeyEvent>            asKeyPress() const;
		std::optional<KeyEvent>            asKeyRelease() const;
		std::optional<TextEvent>           asTextEntered() const;
		std::optional<MouseButtonEvent>    asMouseButtonPress() const;
		std::optional<MouseButtonEvent>    asMouseButtonRelease() const;
		std::optional<MouseMoveEvent>      asMouseMove() const;
		std::optional<MouseWheelEvent>     asMouseWheel() const;
		std::optional<SizeEvent>           asResize() const;
		std::optional<GamepadButtonEvent>  asGamepadButtonPress() const;
		std::optional<GamepadButtonEvent>  asGamepadButtonRelease() const;
		std::optional<GamepadMoveEvent>    asGamepadMove() const;
		std::optional<GamepadConnectEvent> asGamepadConnect() const;
		std::optional<GamepadConnectEvent> asGamepadDisconnect() const;
		std::optional<TouchEvent>          asTouch() const;
		std::optional<SensorEvent>         asSensor() const;

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_event> m_event;
	};

}

#endif
