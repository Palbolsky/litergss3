#ifndef CGSS_TEXTURE_SERIALIZER_H
#define CGSS_TEXTURE_SERIALIZER_H

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include "TextureLoaderSaver.h"
#include "LiteCGSS/Common/Serializer.h"

namespace cgss {
#ifdef LITECGSS_USE_PHYSFS
	struct TextureAssetSerializeMethod {
		static bool load(sf::Texture& texture, const std::string& filename, sf::Image& image);
		static unsigned int save(const sf::Texture& texture, const std::string& filename);
	};
	using TextureFileSerializeMethod = TextureAssetSerializeMethod;
#else
	struct TextureFilesystemSerializeMethod {
		static bool load(sf::Texture& texture, const std::string& filename, sf::Image& image);
		static unsigned int save(const sf::Texture& texture, const std::string& filename);
	};
	using TextureFileSerializeMethod = TextureFilesystemSerializeMethod;
	
#endif
	using TextureFileSerializer = FileSerializer<TextureFileSerializeMethod, sf::Texture, TextureLoader, TextureSaver>;

	struct TextureMemorySerializeMethod {
		static bool load(sf::Texture& texture, const MemorySerializerData& data, sf::Image& image);
		static MemorySerializerData save(const sf::Texture& texture);
	};

	using TextureMemorySerializer = MemorySerializer<TextureMemorySerializeMethod, sf::Texture, TextureLoader, TextureSaver>;

	struct TextureEmptySerializeMethod {
		static bool load(sf::Texture& texture, unsigned int width, unsigned int height, sf::Image& image);
	};

	using TextureEmptySerializer = Empty2DSerializer<TextureEmptySerializeMethod, sf::Texture, TextureLoader>;
}
#endif