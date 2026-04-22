#include "LiteCGSS/Graphics/Font.h"

#include "LiteCGSS/Backend/ActiveBackend.h"

template <class B>
cgss::basic_Font<B>::basic_Font() :
	m_font(std::make_unique<typename B::native_font>()) {}

template <class B>
cgss::basic_Font<B>::basic_Font(const basic_Font& other) :
	m_font(std::make_unique<typename B::native_font>(*other.m_font)) {}

template <class B>
cgss::basic_Font<B>::basic_Font(basic_Font&& other) noexcept = default;

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(const basic_Font& other) {
	*m_font = *other.m_font;
	return *this;
}

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(basic_Font&& other) noexcept = default;

template <class B>
cgss::basic_Font<B>::~basic_Font() = default;

template <class B>
typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() noexcept { return *m_font; }

template <class B>
const typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() const noexcept { return *m_font; }

template class cgss::basic_Font<cgss::backend::ActiveBackend>;
