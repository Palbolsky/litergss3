#ifndef CGSS_IMAGE_H
#define CGSS_IMAGE_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "Serializers/ImageLoaderSaver.h"

namespace sf {
	class Image;
}

namespace cgss {
	class Rectangle;
	template <class B> class basic_Texture;

	class Image {
		template <class B> friend class basic_Texture;
	public:
		Image();
		~Image();

		Image(const Image& image);
		Image(Image&& image) noexcept;
		Image& operator=(const Image& image);
		Image& operator=(Image&& image) noexcept;

		static Image create(const std::string& filename);
		static Image create(const char* rawMemory, std::size_t rawMemorySize);

		bool load(const ImageLoader& loader);
		unsigned int write(ImageSaver& saver) const;

		void blit(const Image& source, unsigned int x, unsigned int y, const Rectangle& rect, bool applyAlpha = false);
		void stretchBlit(const Image& imageSource, const Rectangle& destinationRect, const Rectangle& sourceRect, bool mixedAlpha = false);
		void fillRect(const cgss::Color& color, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

		unsigned int width() const;
		unsigned int height() const;
		cgss::IntRect box() const;
		std::optional<cgss::Color> getPixel(unsigned int x, unsigned int y) const;
		bool setPixel(unsigned int x, unsigned int y, const cgss::Color& color);

		void createMaskFromColor(cgss::Color color, unsigned char alpha);

		const sf::Image& raw() const noexcept;

	private:
		std::unique_ptr<sf::Image> m_image;
	};
}

#endif
