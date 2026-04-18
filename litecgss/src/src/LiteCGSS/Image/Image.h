#ifndef CGSS_IMAGE_H
#define CGSS_IMAGE_H

#include <cstddef>
#include <optional>
#include <SFML/Graphics/Image.hpp>
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "Serializers/ImageLoaderSaver.h"

namespace cgss {
	class Rectangle;
	class Texture;

	class Image {
		friend class Texture;
	public:
		Image() = default;
		~Image() = default;

		Image(const Image& image);
		Image& operator=(const Image& image);

		static Image create(const std::string& filename);
		static Image create(const char* rawMemory, std::size_t rawMemorySize);

		bool load(const ImageLoader& loader);
		unsigned int write(ImageSaver& saver) const;

		void blit(const sf::Image& source, unsigned int x, unsigned int y, const Rectangle& rect, bool applyAlpha = false);
		void stretchBlit(const sf::Image& imageSource, const Rectangle& destinationRect, const Rectangle& sourceRect, bool mixedAlpha = false);
		void fillRect(const cgss::Color& color, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

		unsigned int width() const;
		unsigned int height() const;
		cgss::IntRect box() const;
		std::optional<cgss::Color> getPixel(unsigned int x, unsigned int y) const;
		bool setPixel(unsigned int x, unsigned int y, const cgss::Color& color);

		void createMaskFromColor(cgss::Color color, unsigned char alpha);

		const sf::Image& raw() const { return m_image; }

	private:
		sf::Image m_image;
	};
}

#endif