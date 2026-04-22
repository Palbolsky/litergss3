#include "DrawState.h"

#include "LiteCGSS/Backend/ActiveBackend.h"

template <class B>
cgss::basic_DrawState<B>::basic_DrawState() :
	m_raw(std::make_unique<typename B::native_render_states>()) {}

template <class B>
cgss::basic_DrawState<B>::basic_DrawState(const native_type& raw) :
	m_raw(std::make_unique<typename B::native_render_states>(raw)) {}

template <class B>
cgss::basic_DrawState<B>::basic_DrawState(const basic_DrawState& other) :
	m_raw(std::make_unique<typename B::native_render_states>(*other.m_raw)) {}

template <class B>
cgss::basic_DrawState<B>::basic_DrawState(basic_DrawState&& other) noexcept = default;

template <class B>
cgss::basic_DrawState<B>& cgss::basic_DrawState<B>::operator=(const basic_DrawState& other) {
	*m_raw = *other.m_raw;
	return *this;
}

template <class B>
cgss::basic_DrawState<B>& cgss::basic_DrawState<B>::operator=(basic_DrawState&& other) noexcept = default;

template <class B>
cgss::basic_DrawState<B>::~basic_DrawState() = default;

template <class B>
typename cgss::basic_DrawState<B>::native_type& cgss::basic_DrawState<B>::raw() noexcept { return *m_raw; }

template <class B>
const typename cgss::basic_DrawState<B>::native_type& cgss::basic_DrawState<B>::raw() const noexcept { return *m_raw; }

template class cgss::basic_DrawState<cgss::backend::ActiveBackend>;
