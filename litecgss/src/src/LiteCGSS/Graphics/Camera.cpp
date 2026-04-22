#include "Camera.h"

#include "LiteCGSS/Backend/ActiveBackend.h"

template <class B>
cgss::basic_Camera<B>::basic_Camera() :
	m_view(std::make_unique<typename B::native_view>()) {}

template <class B>
cgss::basic_Camera<B>::basic_Camera(const native_type& view) :
	m_view(std::make_unique<typename B::native_view>(view)) {}

template <class B>
cgss::basic_Camera<B>::basic_Camera(const basic_Camera& other) :
	m_view(std::make_unique<typename B::native_view>(*other.m_view)) {}

template <class B>
cgss::basic_Camera<B>::basic_Camera(basic_Camera&& other) noexcept = default;

template <class B>
cgss::basic_Camera<B>& cgss::basic_Camera<B>::operator=(const basic_Camera& other) {
	*m_view = *other.m_view;
	return *this;
}

template <class B>
cgss::basic_Camera<B>& cgss::basic_Camera<B>::operator=(basic_Camera&& other) noexcept = default;

template <class B>
cgss::basic_Camera<B>::~basic_Camera() = default;

template <class B>
typename cgss::basic_Camera<B>::native_type& cgss::basic_Camera<B>::native() noexcept { return *m_view; }

template <class B>
const typename cgss::basic_Camera<B>::native_type& cgss::basic_Camera<B>::native() const noexcept { return *m_view; }

template class cgss::basic_Camera<cgss::backend::ActiveBackend>;
