#include "Texture.h"

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "LiteCGSS/Image/Image.h"
#include "LiteCGSS/Views/DisplayWindow.h"

// All basic_Texture<B> bodies live here; the explicit instantiation at the
// bottom emits symbols for the build-selected backend. Consumers see the
// template declaration in Texture.h and link to the symbols emitted here.

template <class B>
cgss::basic_Texture<B>::basic_Texture() :
	m_texture(std::make_shared<typename B::native_texture>()),
	m_image(std::make_unique<typename B::native_image>()) {
}

template <class B>
cgss::basic_Texture<B>::basic_Texture(const basic_Texture& other) :
	m_loaded(other.m_loaded),
	m_texture(other.m_texture),
	m_image(std::make_unique<typename B::native_image>(*other.m_image)) {
}

template <class B>
cgss::basic_Texture<B>::basic_Texture(basic_Texture&& other) noexcept = default;

template <class B>
cgss::basic_Texture<B>& cgss::basic_Texture<B>::operator=(const basic_Texture& other) {
	m_loaded = other.m_loaded;
	m_texture = other.m_texture;
	B::Ops::copy_image(*m_image, *other.m_image);
	return *this;
}

template <class B>
cgss::basic_Texture<B>& cgss::basic_Texture<B>::operator=(basic_Texture&& other) noexcept = default;

template <class B>
cgss::basic_Texture<B>::~basic_Texture() = default;

template <class B>
cgss::basic_Texture<B> cgss::basic_Texture<B>::create(const DisplayWindow& window, const std::string& filename) {
	auto result = basic_Texture{};
	B::Ops::set_smooth(*result.m_texture, window.smoothScreen());
	TextureFileSerializeMethod::load(*result.m_texture, filename, *result.m_image);
	return result;
}

template <class B>
cgss::basic_Texture<B> cgss::basic_Texture<B>::create(const std::string& filename) {
	auto result = basic_Texture{};
	TextureFileSerializeMethod::load(*result.m_texture, filename, *result.m_image);
	return result;
}

template <class B>
cgss::basic_Texture<B> cgss::basic_Texture<B>::create_from_native(typename B::native_texture native) {
	auto result = basic_Texture{};
	result.m_texture = std::make_shared<typename B::native_texture>(std::move(native));
	result.m_loaded = true;
	return result;
}

template <class B>
void cgss::basic_Texture<B>::setSmooth(bool smooth) {
	B::Ops::set_smooth(*m_texture, smooth);
}

template <class B>
bool cgss::basic_Texture<B>::load(const TextureLoader& loader) {
	m_loaded = loader.load(*m_texture, *m_image);
	return m_loaded;
}

template <class B>
cgss::basic_Texture<B> cgss::basic_Texture<B>::clone() const {
	auto result = basic_Texture{};
	result.m_texture = m_texture;
	return result;
}

template <class B>
void cgss::basic_Texture<B>::update() {
	B::Ops::update_texture_from_image(*m_texture, *m_image);
}

template <class B>
void cgss::basic_Texture<B>::update(const basic_Image<B>& image) {
	// Explicit copy from the provided cgss::Image's backing native image.
	B::Ops::copy_image(*m_image, *image.m_image);
	B::Ops::update_texture_from_image(*m_texture, *m_image);
}

template <class B>
void cgss::basic_Texture<B>::blit(unsigned long x, unsigned long y, const basic_Texture& srcTexture, const Rectangle& rect) {
	if (hasImage()) {
		B::Ops::image_blit(*m_image, x, y, *srcTexture.m_image, rect.getValue());
	}
}

template <class B>
void cgss::basic_Texture<B>::fillRect(long x, long y, unsigned int width, unsigned int height, cgss::Color color) {
	const long xStart = x < 0 ? 0 : x;
	const long xEnd = x + width;
	const long yStart = y < 0 ? 0 : y;
	const long yEnd = y + height;

	for (long yIt = yStart; yIt < yEnd; yIt++) {
		for (long xIt = xStart; xIt < xEnd; xIt++) {
			B::Ops::image_set_pixel(*m_image, static_cast<unsigned int>(xIt), static_cast<unsigned int>(yIt), color);
		}
	}
	update();
}

template <class B>
void cgss::basic_Texture<B>::clearRect(long x, long y, unsigned int width, unsigned int height) {
	fillRect(x, y, width, height, cgss::Colors::Transparent);
}

template <class B>
unsigned int cgss::basic_Texture<B>::write(TextureSaver& saver) const {
	return saver.save(*m_texture);
}

template <class B>
unsigned int cgss::basic_Texture<B>::getNpotSize(unsigned int size) {
	unsigned int powerOfTwo = 1;
	while (powerOfTwo < size) {
		powerOfTwo *= 2;
	}
	return powerOfTwo;
}

template <class B>
typename cgss::basic_Texture<B>::Size cgss::basic_Texture<B>::getSize() const {
	const auto sz = B::Ops::get_texture_size(*m_texture);
	return { sz.x, sz.y };
}

template <class B>
bool cgss::basic_Texture<B>::create(int sw, int sh) {
	return B::Ops::create_texture(*m_texture, static_cast<unsigned int>(sw), static_cast<unsigned int>(sh));
}

template <class B>
std::weak_ptr<typename cgss::basic_Texture<B>::native_type> cgss::basic_Texture<B>::weak() { return m_texture; }

template <class B>
std::shared_ptr<typename cgss::basic_Texture<B>::native_type> cgss::basic_Texture<B>::ref() { return m_texture; }

template <class B>
typename cgss::basic_Texture<B>::native_type& cgss::basic_Texture<B>::native() { return *m_texture.get(); }

template <class B>
const typename cgss::basic_Texture<B>::native_type& cgss::basic_Texture<B>::native() const { return *m_texture.get(); }

template <class B>
cgss::TextureFactorNpot cgss::basic_Texture<B>::factorNpot() const {
	const auto sz = B::Ops::get_texture_size(*m_texture);
	return {
		static_cast<double>(sz.x) / getNpotSize(sz.x),
		static_cast<double>(sz.y) / getNpotSize(sz.y)
	};
}

template <class B>
bool cgss::basic_Texture<B>::hasImage() const {
	return B::Ops::get_image_size(*m_image).x != 0;
}

// Explicit instantiation for the build-selected backend. The wrapper has
// no other instantiations in this build, so all symbols are emitted here
// and consumers' TUs link against them.
template class cgss::basic_Texture<cgss::backend::ActiveBackend>;
