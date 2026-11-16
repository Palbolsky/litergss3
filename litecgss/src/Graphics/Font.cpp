#include "LiteCGSS/Graphics/Font.h"

#include "Backend/ActiveBackend.h"

template <class B>
cgss::basic_Font<B>::basic_Font() :
	m_font(std::make_unique<typename B::native_font>()) {}

template <class B>
cgss::basic_Font<B>::basic_Font(const basic_Font& other) :
	m_font(std::make_unique<typename B::native_font>(*other.m_font)),
	m_loaded(other.m_loaded) {}

template <class B>
cgss::basic_Font<B>::basic_Font(basic_Font&& other) noexcept = default;

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(const basic_Font& other) {
	// Release any existing backing before overwrite (raylib: GPU texture).
	if (m_loaded) B::Ops::font_destroy(*m_font);
	*m_font = *other.m_font;
	m_loaded = other.m_loaded;
	return *this;
}

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(basic_Font&& other) noexcept = default;

// Custom destructor — mirrors basic_Image. Raylib's `::Font` POD has no
// dtor-side release of its GPU atlas; route through Ops::font_destroy so
// the backing is freed on the last copy going out of scope.
template <class B>
cgss::basic_Font<B>::~basic_Font() {
	if (m_font && m_loaded) {
		B::Ops::font_destroy(*m_font);
	}
}

template <class B>
bool cgss::basic_Font<B>::load(const std::string& filename) {
	// If we're replacing an existing load, release the previous backing
	// first — prevents GPU-atlas leaks on repeated loads into the same
	// cgss::Font instance.
	if (m_loaded) B::Ops::font_destroy(*m_font);
	m_loaded = B::Ops::font_load_from_file(*m_font, filename.c_str());
	return m_loaded;
}

template <class B>
bool cgss::basic_Font<B>::load(const std::string& filename, unsigned int fontSize) {
	if (m_loaded) B::Ops::font_destroy(*m_font);
	m_loaded = B::Ops::font_load_from_file_with_size(*m_font, filename.c_str(),
	                                                  static_cast<int>(fontSize));
	return m_loaded;
}

template <class B>
typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() noexcept { return *m_font; }

template <class B>
const typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() const noexcept { return *m_font; }

template class cgss::basic_Font<cgss::backend::ActiveBackend>;
