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

template class cgss::basic_Event<cgss::backend::ActiveBackend>;
