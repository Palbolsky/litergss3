#include "LiteCGSS/Graphics/Font.h"

#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Common/VFS.h"

template <class B>
cgss::basic_Font<B>::basic_Font() :
	m_font(std::make_unique<typename B::native_font>()) {}

template <class B>
cgss::basic_Font<B>::basic_Font(const basic_Font& other) :
	m_data(other.m_data),
	m_font(std::make_unique<typename B::native_font>(*other.m_font)),
	m_loaded(other.m_loaded) {}

template <class B>
cgss::basic_Font<B>::basic_Font(basic_Font&& other) noexcept = default;

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(const basic_Font& other) {
	// Release any existing backing (e.g. a GPU atlas) before overwrite.
	if (m_loaded) B::Ops::font_destroy(*m_font);
	*m_font = *other.m_font;
	m_data = other.m_data;
	m_loaded = other.m_loaded;
	return *this;
}

template <class B>
cgss::basic_Font<B>& cgss::basic_Font<B>::operator=(basic_Font&& other) noexcept = default;

// Custom destructor — mirrors basic_Image. A native font is not guaranteed
// to release its GPU atlas on destruction; route through Ops::font_destroy
// so the backing is freed on the last copy going out of scope.
template <class B>
cgss::basic_Font<B>::~basic_Font() {
	if (m_font && m_loaded) {
		B::Ops::font_destroy(*m_font);
	}
}

template <class B>
bool cgss::basic_Font<B>::load(const std::string& filename) {
	return loadFile(filename, 0);
}

template <class B>
bool cgss::basic_Font<B>::load(const std::string& filename, unsigned int fontSize) {
	return loadFile(filename, static_cast<int>(fontSize));
}

// fontSize 0: the backend's default atlas size
template <class B>
bool cgss::basic_Font<B>::loadFile(const std::string& filename, int fontSize) {
	// If we're replacing an existing load, release the previous backing
	// first — prevents GPU-atlas leaks on repeated loads into the same
	// cgss::Font instance.
	if (m_loaded) B::Ops::font_destroy(*m_font);
#ifdef LITECGSS_USE_PHYSFS
	// A game running from a mounted archive has its fonts there only, as
	// its images (Image loads through the VFS too)
	if (cgss::vfs::exists(filename)) {
		auto data = std::make_shared<const std::vector<char>>(cgss::vfs::loadFully(filename));
		m_loaded = fontSize > 0
			? B::Ops::font_load_from_memory_with_size(*m_font, data->data(), data->size(), fontSize)
			: B::Ops::font_load_from_memory(*m_font, data->data(), data->size());
		// After the load: the previous bytes outlive the face that used them
		m_data = m_loaded ? std::move(data) : nullptr;
		return m_loaded;
	}
#endif
	m_loaded = fontSize > 0
		? B::Ops::font_load_from_file_with_size(*m_font, filename.c_str(), fontSize)
		: B::Ops::font_load_from_file(*m_font, filename.c_str());
	m_data = nullptr;
	return m_loaded;
}

template <class B>
typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() noexcept { return *m_font; }

template <class B>
const typename cgss::basic_Font<B>::native_type& cgss::basic_Font<B>::native() const noexcept { return *m_font; }

template class cgss::basic_Font<cgss::backend::ActiveBackend>;
