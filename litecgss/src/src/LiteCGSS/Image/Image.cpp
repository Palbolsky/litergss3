#include "LiteCGSS/Common/Rectangle.h"
#include "Serializers/ImageSerializer.h"
#include "Image.h"

cgss::Image::Image(const Image& image) {
	*this = image;
}

cgss::Image& cgss::Image::operator=(const Image& image) {
	m_image.create(image.m_image.getSize().x, image.m_image.getSize().y, image.m_image.getPixelsPtr());
	return *this;
}

cgss::Image cgss::Image::create(const std::string& filename) {
	auto image = cgss::Image {};
	ImageFileSerializeMethod::load(image.m_image, filename);
	return image;
}

cgss::Image cgss::Image::create(const char* rawMemory, std::size_t rawMemorySize) {
	auto image = cgss::Image {};
	const MemorySerializerData data {reinterpret_cast<unsigned char*>(const_cast<char*>(rawMemory)), rawMemorySize};
	ImageMemorySerializeMethod::load(image.m_image, data);
	return image;
}

bool cgss::Image::load(const ImageLoader& loader) {
	return loader.load(m_image);
}

unsigned int cgss::Image::width() const {
	return m_image.getSize().x;
}

unsigned int cgss::Image::height() const {
	return m_image.getSize().y;
}

sf::IntRect cgss::Image::box() const {
	const auto size = m_image.getSize();
	return sf::IntRect {0, 0, static_cast<int>(size.x), static_cast<int>(size.y) };
}

void cgss::Image::blit(const sf::Image& source, unsigned int x, unsigned int y, const Rectangle& rect, bool applyAlpha) {
	m_image.copy(
		source,
		x,
		y,
		rect.getValue(),
		applyAlpha
	);
}

void cgss::Image::fillRect(const sf::Color& color, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
	const auto x2 = width + x;
	const auto y2 = height + y;
	for (auto yIt = y; yIt < y2; yIt++) {
		for (auto xIt = x; xIt < x2; xIt++) {
			m_image.setPixel(xIt, yIt, color);
		}
	}
}

unsigned int cgss::Image::write(ImageSaver& saver) const {
	return saver.save(m_image);
}

std::optional<sf::Color> cgss::Image::getPixel(unsigned int x, unsigned int y) const {
	const auto& size = m_image.getSize();
	if (x < size.x && y < size.y) {
		return m_image.getPixel(x, y);
	}
	return {};
}

bool cgss::Image::setPixel(unsigned int x, unsigned int y, const sf::Color& color) {
	const auto& size = m_image.getSize();
	if (x < size.x && y < size.y) {
		m_image.setPixel(x, y, color);
		return true;
	}
	return false;
}

void cgss::Image::stretchBlit(const sf::Image& imageSource, const Rectangle& destinationRect, const Rectangle& sourceRect, bool mixedAlpha) {
	const auto& dest = destinationRect.getValue();
	const auto& src = sourceRect.getValue();

	const auto& sourceImageSize = imageSource.getSize();
	const auto& destinationImageSize = m_image.getSize();

	for (int y = 0; y < dest.height; y++) {
		const auto d_y = y + dest.top;
		if (d_y >= static_cast<int>(destinationImageSize.y)) { break; }
		if (d_y < 0) { continue; }

		const auto s_y = y * src.height / dest.height + src.top;
		if (s_y >= static_cast<int>(sourceImageSize.y)) { break; }
		if (s_y < 0) { continue; }

		for (int x = 0; x < dest.width; x++) {
			const auto d_x = x + dest.left;
			if (d_x >= static_cast<int>(destinationImageSize.x)) { break; }
			if (d_x < 0) { continue; }

			const auto s_x = x * src.width / dest.width + src.left;
			if (s_x >= static_cast<int>(sourceImageSize.x)) { break; }
			if (s_x < 0) { continue; }

			if (mixedAlpha) {
				auto srcColor = imageSource.getPixel(s_x, s_y);
				const auto destColor = m_image.getPixel(d_x, d_y);
				if (destColor.a > 0) {
					unsigned char mina = 255 - srcColor.a;
					srcColor.r = (srcColor.r * srcColor.a + destColor.r * mina) / 255;
					srcColor.g = (srcColor.g * srcColor.a + destColor.g * mina) / 255;
					srcColor.b = (srcColor.b * srcColor.a + destColor.b * mina) / 255;
					srcColor.a += (destColor.a * mina / 255);
				}
				m_image.setPixel(d_x, d_y, srcColor);
			} else {
				m_image.setPixel(d_x, d_y, imageSource.getPixel(s_x, s_y));
			}
		}
	}
}

void cgss::Image::createMaskFromColor(sf::Color color, unsigned char alpha) {
	m_image.createMaskFromColor(color, alpha);
}
