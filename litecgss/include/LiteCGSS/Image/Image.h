#ifndef CGSS_IMAGE_H
#define CGSS_IMAGE_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Image/ImageFwd.h"
#include "LiteCGSS/Image/Serializers/ImageLoaderSaver.h"

namespace cgss {
	class Rectangle;
	template <class B> class basic_Texture;

	template <class B>
	class basic_Image {
		template <class C> friend class basic_Texture;
		friend struct cgss::backend::ActiveBackend::Ops;
	public:
		using native_type = typename B::native_image;

		basic_Image();
		~basic_Image();

		basic_Image(const basic_Image& image);
		basic_Image(basic_Image&& image) noexcept;
		basic_Image& operator=(const basic_Image& image);
		basic_Image& operator=(basic_Image&& image) noexcept;

		static basic_Image create(const std::string& filename);
		static basic_Image create(const char* rawMemory, std::size_t rawMemorySize);
		// Empty image of the given dimensions (all pixels transparent).
		// Backed by basic_ImageEmptySerializer under the hood.
		static basic_Image create(unsigned int width, unsigned int height);

		bool load(const ImageLoader& loader);
		unsigned int write(ImageSaver& saver) const;

		void blit(const basic_Image& source, unsigned int x, unsigned int y, const Rectangle& rect, bool applyAlpha = false);
		void stretchBlit(const basic_Image& imageSource, const Rectangle& destinationRect, const Rectangle& sourceRect, bool mixedAlpha = false);
		// IntRect overloads — avoid allocating a Rectangle BondElement for
		// throwaway coordinates. Forward to the same Ops path.
		void blit(const basic_Image& source, unsigned int x, unsigned int y, const cgss::IntRect& rect, bool applyAlpha = false);
		void stretchBlit(const basic_Image& imageSource, const cgss::IntRect& destinationRect, const cgss::IntRect& sourceRect, bool mixedAlpha = false);
		void fillRect(const cgss::Color& color, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

		unsigned int width() const;
		unsigned int height() const;
		cgss::IntRect box() const;
		std::optional<cgss::Color> getPixel(unsigned int x, unsigned int y) const;
		bool setPixel(unsigned int x, unsigned int y, const cgss::Color& color);

		void createMaskFromColor(cgss::Color color, unsigned char alpha);

	private:
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_image> m_image;
	};
}

#endif
