#include "LiteCGSS/Image/Image.h"

#include <type_traits>

#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Backend/SFMLBackendFwd.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Image/Serializers/ImageSerializer.h"

template <class B>
cgss::basic_Image<B>::basic_Image() :
	m_image(std::make_unique<typename B::native_image>()) {}

// Custom destructor — raylib's `::Image` is a POD with no destructor that
// knows to call `UnloadImage`, so pixel backing would leak on a default
// `= default` dtor. Route through `Ops::image_destroy` which each backend
// implements to match its native semantics (SFML no-op: sf::Image's own
// dtor self-releases via unique_ptr<sf::Image>).
template <class B>
cgss::basic_Image<B>::~basic_Image() {
	if (m_image) {
		B::Ops::image_destroy(*m_image);
	}
}

template <class B>
cgss::basic_Image<B>::basic_Image(const basic_Image& image) :
	m_image(std::make_unique<typename B::native_image>()) {
	*this = image;
}

template <class B>
cgss::basic_Image<B>::basic_Image(basic_Image&& image) noexcept = default;

template <class B>
cgss::basic_Image<B>& cgss::basic_Image<B>::operator=(const basic_Image& image) {
	const auto sz = B::Ops::get_image_size(*image.m_image);
	B::Ops::image_create_with_pixels(*m_image, sz.x, sz.y, B::Ops::get_image_pixels_ptr(*image.m_image));
	return *this;
}

template <class B>
cgss::basic_Image<B>& cgss::basic_Image<B>::operator=(basic_Image&& image) noexcept = default;

template <class B>
cgss::basic_Image<B> cgss::basic_Image<B>::create(const std::string& filename) {
	auto image = basic_Image{};
	basic_ImageFileSerializeMethod<B>::load(*image.m_image, filename);
	return image;
}

template <class B>
cgss::basic_Image<B> cgss::basic_Image<B>::create(const char* rawMemory, std::size_t rawMemorySize) {
	auto image = basic_Image{};
	const MemorySerializerData data {
		reinterpret_cast<unsigned char*>(const_cast<char*>(rawMemory)), rawMemorySize };
	basic_ImageMemorySerializeMethod<B>::load(*image.m_image, data);
	return image;
}

template <class B>
cgss::basic_Image<B> cgss::basic_Image<B>::create(unsigned int width, unsigned int height) {
	auto image = basic_Image{};
	basic_ImageEmptySerializeMethod<B>::load(*image.m_image, width, height);
	return image;
}

template <class B>
bool cgss::basic_Image<B>::load(const ImageLoader& loader) {
	return loader.load(*m_image);
}

template <class B>
unsigned int cgss::basic_Image<B>::width() const {
	return B::Ops::get_image_size(*m_image).x;
}

template <class B>
unsigned int cgss::basic_Image<B>::height() const {
	return B::Ops::get_image_size(*m_image).y;
}

template <class B>
cgss::IntRect cgss::basic_Image<B>::box() const {
	const auto sz = B::Ops::get_image_size(*m_image);
	return cgss::IntRect{ 0, 0, static_cast<int>(sz.x), static_cast<int>(sz.y) };
}

template <class B>
void cgss::basic_Image<B>::blit(const basic_Image& source, unsigned int x, unsigned int y, const Rectangle& rect, bool applyAlpha) {
	B::Ops::image_blit(*m_image, x, y, *source.m_image, rect.getValue(), applyAlpha);
}

template <class B>
void cgss::basic_Image<B>::blit(const basic_Image& source, unsigned int x, unsigned int y, const cgss::IntRect& rect, bool applyAlpha) {
	B::Ops::image_blit(*m_image, x, y, *source.m_image, rect, applyAlpha);
}

template <class B>
void cgss::basic_Image<B>::fillRect(const cgss::Color& color, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
	const auto x2 = width + x;
	const auto y2 = height + y;
	for (auto yIt = y; yIt < y2; yIt++) {
		for (auto xIt = x; xIt < x2; xIt++) {
			B::Ops::image_set_pixel(*m_image, xIt, yIt, color);
		}
	}
}

template <class B>
unsigned int cgss::basic_Image<B>::write(ImageSaver& saver) const {
	return saver.save(*m_image);
}

template <class B>
std::optional<cgss::Color> cgss::basic_Image<B>::getPixel(unsigned int x, unsigned int y) const {
	const auto sz = B::Ops::get_image_size(*m_image);
	if (x < sz.x && y < sz.y) {
		return B::Ops::image_get_pixel(*m_image, x, y);
	}
	return {};
}

template <class B>
bool cgss::basic_Image<B>::setPixel(unsigned int x, unsigned int y, const cgss::Color& color) {
	const auto sz = B::Ops::get_image_size(*m_image);
	if (x < sz.x && y < sz.y) {
		B::Ops::image_set_pixel(*m_image, x, y, color);
		return true;
	}
	return false;
}

template <class B>
void cgss::basic_Image<B>::stretchBlit(const basic_Image& imageSource, const cgss::IntRect& destinationRect, const cgss::IntRect& sourceRect, bool mixedAlpha) {
	// Delegate to the Rectangle overload by constructing BondElement
	// wrappers — the per-pixel loop below is identical in either form.
	cgss::Rectangle dr; dr.setValue(destinationRect);
	cgss::Rectangle sr; sr.setValue(sourceRect);
	stretchBlit(imageSource, dr, sr, mixedAlpha);
}

template <class B>
void cgss::basic_Image<B>::stretchBlit(const basic_Image& imageSource, const Rectangle& destinationRect, const Rectangle& sourceRect, bool mixedAlpha) {
	const auto& dest = destinationRect.getValue();
	const auto& src = sourceRect.getValue();

	const auto srcSize  = B::Ops::get_image_size(*imageSource.m_image);
	const auto destSize = B::Ops::get_image_size(*m_image);

	for (int y = 0; y < dest.height; y++) {
		const auto d_y = y + dest.top;
		if (d_y >= static_cast<int>(destSize.y)) { break; }
		if (d_y < 0) { continue; }

		const auto s_y = y * src.height / dest.height + src.top;
		if (s_y >= static_cast<int>(srcSize.y)) { break; }
		if (s_y < 0) { continue; }

		for (int x = 0; x < dest.width; x++) {
			const auto d_x = x + dest.left;
			if (d_x >= static_cast<int>(destSize.x)) { break; }
			if (d_x < 0) { continue; }

			const auto s_x = x * src.width / dest.width + src.left;
			if (s_x >= static_cast<int>(srcSize.x)) { break; }
			if (s_x < 0) { continue; }

			if (mixedAlpha) {
				auto srcColor = B::Ops::image_get_pixel(*imageSource.m_image, s_x, s_y);
				const auto destColor = B::Ops::image_get_pixel(*m_image, d_x, d_y);
				if (destColor.a > 0) {
					unsigned char mina = 255 - srcColor.a;
					srcColor.r = (srcColor.r * srcColor.a + destColor.r * mina) / 255;
					srcColor.g = (srcColor.g * srcColor.a + destColor.g * mina) / 255;
					srcColor.b = (srcColor.b * srcColor.a + destColor.b * mina) / 255;
					srcColor.a += (destColor.a * mina / 255);
				}
				B::Ops::image_set_pixel(*m_image, d_x, d_y, srcColor);
			} else {
				B::Ops::image_set_pixel(*m_image, d_x, d_y,
					B::Ops::image_get_pixel(*imageSource.m_image, s_x, s_y));
			}
		}
	}
}

template <class B>
void cgss::basic_Image<B>::createMaskFromColor(cgss::Color color, unsigned char alpha) {
	B::Ops::image_create_mask_from_color(*m_image, color, alpha);
}

template <class B>
const typename cgss::basic_Image<B>::native_type& cgss::basic_Image<B>::native() const noexcept {
	return *m_image;
}

template class cgss::basic_Image<cgss::backend::ActiveBackend>;
