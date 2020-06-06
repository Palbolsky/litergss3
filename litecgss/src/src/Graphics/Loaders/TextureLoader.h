#ifndef CGSS_TEXTURE_LOADER_H
#define CGSS_TEXTURE_LOADER_H

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>

namespace cgss {
	
	class TextureLoader {
	public:
		TextureLoader() = default;
		virtual ~TextureLoader() = default;
		virtual bool load(sf::Texture& texture, sf::Image& image) const = 0;
	protected:
		static bool lodePngDecode(sf::Texture& texture, const char* rawDataOrFilename, long from_memory_size);
	};

	class TextureSaver {
	public:
		TextureSaver() = default;
		virtual ~TextureSaver() = default;
		virtual void save(const sf::Texture& texture) = 0;
	};

	class TextureFileLoader : 
		public TextureLoader,
		public TextureSaver {
	public:
		TextureFileLoader(std::string filename);
		bool load(sf::Texture& texture, sf::Image& image) const override;
		void save(const sf::Texture& texture) override;
		static bool instantLoad(const std::string& filename, sf::Texture& texture, sf::Image& image);
		static unsigned int instantSave(const std::string& filename, const sf::Texture& texture);
		unsigned int writtenStatus() const { return m_writtenStatus; }
	private:
		std::string m_filename;
		unsigned int m_writtenStatus = 0;
	};

	class TextureMemoryLoader :
		public TextureLoader,
		public TextureSaver {
	public:
		TextureMemoryLoader(std::vector<unsigned char> data);
		bool load(sf::Texture& texture, sf::Image& image) const override;
		void save(const sf::Texture& texture) override;
		static bool instantLoad(const std::vector<unsigned char>& data, sf::Texture& texture, sf::Image& image);
		static std::vector<unsigned char> instantSave(const sf::Texture& texture);
		std::vector<unsigned char>&& stealMemory() { return std::move(m_memory); }
	private:
		std::vector<unsigned char> m_memory;
	};

	class TextureEmptyLoader :
		public TextureLoader {
	public:
		TextureEmptyLoader(unsigned int width, unsigned int height);
		bool load(sf::Texture& texture, sf::Image& image) const override;
	private:
		unsigned int m_width;
		unsigned int m_height;
	};
	
}
#endif