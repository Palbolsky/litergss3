#ifndef CGSS_TEXTURE_H
#define CGSS_TEXTURE_H

#include <vector>
#include <SFML/Graphics.hpp>
#include "Common/Meta/metadata.h"

namespace cgss {

	enum class TextureLoadMethod {
		Memory,
		File,
		New
	};

	class Rectangle;
	class Shape;

	class Texture {
	public:
		Texture() = default;
		~Texture() = default;

		void update();
		auto getSize() const { return m_texture.getSize(); }

		void blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect);
		void fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color);
		void clearRect(long x, long y, unsigned int width, unsigned int height);

		void sync();

		//Conversions (could be externalized)
		bool load(const std::string& rawDataOrFilename, unsigned int width, unsigned int height, TextureLoadMethod loadMethod);
		std::vector<unsigned char> toRawDataPng() const;
		int toFile(const std::string& filename) const;

		bool isLoaded() const { return m_loaded; }

		void setShaderUniform(sf::Shader&, const std::string& attribute) const;
		void update(const sf::Window& window, unsigned int x, unsigned int y) { m_texture.update(window, x, y); }
		bool create(int sw, int sh) { return m_texture.create(sw, sh); }
		std::unique_ptr<sf::Sprite> createSprite();
		void linkShape(sf::Shape& shape);

	private:
		static bool loadPng(sf::Texture& output, const char* rawDataOrFilename, long fromMemorySize);

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