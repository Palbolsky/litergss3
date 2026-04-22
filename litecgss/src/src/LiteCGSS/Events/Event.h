#ifndef CGSS_EVENT_H
#define CGSS_EVENT_H

#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Events/EventFwd.h"

namespace cgss {

	// Templated wrapper over `Backend::native_event`. Consumers refer to it
	// as `cgss::Event`. The native event handle is reachable only via
	// `Backend::Ops::native_of(event)` from impl-side .cpps that have the
	// full backend header included; consumers cannot escape the abstraction.
	template <class B>
	class basic_Event {
		friend struct cgss::backend::SFMLBackend::Ops;
	public:
		using native_type = typename B::native_event;

		basic_Event();
		basic_Event(const basic_Event& other);
		basic_Event(basic_Event&& other) noexcept;
		basic_Event& operator=(const basic_Event& other);
		basic_Event& operator=(basic_Event&& other) noexcept;
		~basic_Event();

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_event> m_event;
	};

}

#endif
