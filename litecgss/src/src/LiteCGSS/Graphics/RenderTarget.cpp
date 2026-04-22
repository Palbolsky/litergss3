#include "RenderTarget.h"

#include "LiteCGSS/Backend/ActiveBackend.h"

template <class B>
cgss::basic_RenderTarget<B>::basic_RenderTarget(native_type& target) noexcept :
	m_target(&target) {}

template <class B>
cgss::basic_RenderTarget<B>::basic_RenderTarget(const basic_RenderTarget&) noexcept = default;

template <class B>
cgss::basic_RenderTarget<B>& cgss::basic_RenderTarget<B>::operator=(const basic_RenderTarget&) noexcept = default;

template <class B>
cgss::basic_RenderTarget<B>::basic_RenderTarget(basic_RenderTarget&&) noexcept = default;

template <class B>
cgss::basic_RenderTarget<B>& cgss::basic_RenderTarget<B>::operator=(basic_RenderTarget&&) noexcept = default;

template <class B>
typename cgss::basic_RenderTarget<B>::native_type& cgss::basic_RenderTarget<B>::native() const noexcept {
	return *m_target;
}

template class cgss::basic_RenderTarget<cgss::backend::ActiveBackend>;
