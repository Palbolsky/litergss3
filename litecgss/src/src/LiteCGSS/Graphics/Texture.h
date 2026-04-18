#ifndef CGSS_TEXTURE_H
#define CGSS_TEXTURE_H

#include <vector>
#include <array>
#include <memory>
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "Serializers/TextureLoaderSaver.h"

namespace sf {
	class Texture;
	class Image;
}

namespace cgss {
	class Rectangle;
	class Shape;
	class DisplayWindow;
	class Image;

	using TextureFactorNpot = std::array<double, 2>;

	class Texture {
	public:
		Texture();
		Texture(const Texture& other);
		Texture(Texture&& other) noexcept;
		Texture& operator=(const Texture& other);
		Texture& operator=(Texture&& other) noexcept;
		virtual ~Texture();

		static Texture create(const DisplayWindow& window, const std::string& filename);
		static Texture create(const std::string& filename);
		static Texture create(sf::Texture texture);

		void update(const sf::Image* image = nullptr);
		void update(const Image& image);

		struct Size { unsigned int x; unsigned int y; };
		Size getSize() const;

		void blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect);
		void fillRect(long x, long y, unsigned int width, unsigned int height, cgss::Color color);
		void clearRect(long x, long y, unsigned int width, unsigned int height);
		void setSmooth(bool smooth);

		Texture clone() const;

		bool load(const TextureLoader& loader);
		unsigned int write(TextureSaver& saver) const;

		bool isLoaded() const { return m_loaded; }

		bool create(int sw, int sh);

		std::weak_ptr<sf::Texture> weak();
		std::shared_ptr<sf::Texture> ref();
		sf::Texture& raw();
		const sf::Texture& raw() const;

		TextureFactorNpot factorNpot() const;

	private:
		static unsigned int getNpotSize(unsigned int size);
		bool hasImage() const;

		bool m_loaded = false;
		std::shared_ptr<sf::Texture> m_texture;
		std::unique_ptr<sf::Image> m_image;
	};

	namespace meta {
		template<>
		struct Log<Texture> {
			static constexpr auto classname = "Texture";
		};
	}
}

#endif
