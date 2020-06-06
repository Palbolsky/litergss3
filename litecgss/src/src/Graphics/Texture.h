#ifndef CGSS_TEXTURE_H
#define CGSS_TEXTURE_H

#include <vector>
#include <SFML/Graphics.hpp>
#include "Common/Meta/metadata.h"

namespace cgss {

	class Rectangle;
	class Shape;
	class DisplayWindow;
	class TextureLoader;
	class TextureSaver;

	class Texture {
	public:
		Texture() = default;
		~Texture() = default;

		static Texture create(const DisplayWindow& window, const std::string& filename);
		static Texture create(const std::string& filename);

		void update();
		auto getSize() const { return m_texture.getSize(); }

		void blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect);
		void fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color);
		void clearRect(long x, long y, unsigned int width, unsigned int height);
		void setSmooth(bool smooth);
		
		Texture clone() const;

		void sync();

		bool load(const TextureLoader& loader);
		void write(TextureSaver& saver) const;

		std::string toRawDataPng() const;
		unsigned int toFile(const std::string& filename) const;

		bool isLoaded() const { return m_loaded; }

		void update(const sf::Window& window, unsigned int x, unsigned int y) { m_texture.update(window, x, y); }
		bool create(int sw, int sh) { return m_texture.create(sw, sh); }

		sf::Texture& getTexture() { return m_texture; }
		const sf::Texture& getTexture() const { return m_texture; }

	private:
		bool hasImage() const { return m_image.getSize().x != 0; };

		bool m_loaded = false;
		sf::Texture m_texture;
		sf::Image m_image;
	};

	namespace meta {
		template<>
		struct Log<Texture> {
			static constexpr auto classname = "Texture";
		};
	}
}

#endif