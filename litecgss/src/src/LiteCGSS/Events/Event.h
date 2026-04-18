#ifndef CGSS_EVENT_H
#define CGSS_EVENT_H

#include <memory>

namespace sf {
	class Event;
}

namespace cgss {

	// Opaque wrapper around the backend window event (currently sf::Event).
	// Uses pimpl so that public LiteCGSS headers do not need to include
	// <SFML/Window/Event.hpp>. Callers that need to inspect the event payload
	// (event type, key code, mouse coords, …) use .raw() at the backend
	// boundary; this escape hatch will disappear once the backend is replaced
	// and a native cgss::Event variant is introduced.
	class Event {
	public:
		Event();
		Event(const Event& other);
		Event(Event&& other) noexcept;
		Event& operator=(const Event& other);
		Event& operator=(Event&& other) noexcept;
		~Event();

		sf::Event& raw() noexcept;
		const sf::Event& raw() const noexcept;

	private:
		std::unique_ptr<sf::Event> m_event;
	};

}

#endif
