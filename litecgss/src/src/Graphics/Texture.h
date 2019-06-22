#ifndef LR_TEXTURE_H
#define LR_TEXTURE_H

#include <vector>
#include <SFML/Graphics.hpp>
#include "Common/Meta/metadata.h"

namespace lr {

	enum class TextureLoadMethod {
		Memory,
		File,
		New
	};

	class Rectangle;
	class Texture {
	public:
		Texture() = default;
		~Texture() = default;

		void update();
		auto getSize() const { return m_texture.getSize(); }

		void blit(unsigned long x, unsigned long y, const Texture& srcTexture, const Rectangle& rect);
		void fillRect(long x, long y, unsigned int width, unsigned int height, sf::Color color);
		void clearRect(long x, long y, unsigned int width, unsigned int height);

		//Conversions (could be externalized)
		bool load(const std::string& rawDataOrFilename, unsigned int width, unsigned int height, TextureLoadMethod loadMethod);
		std::vector<unsigned char> toRawDataPng();
		int toFile(const std::string& filename);

	private:
		static bool loadPng(sf::Texture& output, const char* rawDataOrFilename, long fromMemorySize);

		bool hasImage() const { return m_image.getSize().x != 0; };

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