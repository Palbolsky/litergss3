#ifndef CGSS_EVENT_H
#define CGSS_EVENT_H

#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Events/EventFwd.h"

namespace cgss {

	// Templated wrapper over `Backend::native_event`. Consumers refer to it
	// as `cgss::Event`. Callers that need to inspect the event payload
	// (event type, key code, mouse coords, …) use `raw()` at the backend
	// boundary; this escape hatch will retire in PR L (or earlier, when a
	// native cgss::Event variant is introduced).
	template <class B>
	class basic_Event {
	public:
		using native_type = typename B::native_event;

		basic_Event();
		basic_Event(const basic_Event& other);
		basic_Event(basic_Event&& other) noexcept;
		basic_Event& operator=(const basic_Event& other);
		basic_Event& operator=(basic_Event&& other) noexcept;
		~basic_Event();

		native_type& raw() noexcept;
		const native_type& raw() const noexcept;

	private:
		std::unique_ptr<typename B::native_event> m_event;
	};

}

#endif
