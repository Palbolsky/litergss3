#include "Event.h"
#include <SFML/Window/Event.hpp>

cgss::Event::Event() : m_event(std::make_unique<sf::Event>()) {}

cgss::Event::Event(const Event& other) : m_event(std::make_unique<sf::Event>(*other.m_event)) {}

cgss::Event::Event(Event&& other) noexcept = default;

cgss::Event& cgss::Event::operator=(const Event& other) {
	*m_event = *other.m_event;
	return *this;
}

cgss::Event& cgss::Event::operator=(Event&& other) noexcept = default;

cgss::Event::~Event() = default;

sf::Event& cgss::Event::raw() noexcept { return *m_event; }
const sf::Event& cgss::Event::raw() const noexcept { return *m_event; }
