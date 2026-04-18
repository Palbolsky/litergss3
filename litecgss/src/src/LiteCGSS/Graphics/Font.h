#ifndef CGSS_FONT_H
#define CGSS_FONT_H

#include <memory>

namespace sf {
	class Font;
}

namespace cgss {

	// Opaque wrapper around the backend font (currently sf::Font). Uses pimpl so
	// that public LiteCGSS headers do not need to include <SFML/Graphics/Font.hpp>.
	// Copies perform a deep copy of the underlying sf::Font (which itself owns
	// font data — so copies are relatively expensive).
	//
	// The raw() accessor is a temporary escape hatch used by implementation
	// .cpp files; it will disappear once the backend is replaced.
	class Font {
	public:
		Font();
		Font(const Font& other);
		Font(Font&& other) noexcept;
		Font& operator=(const Font& other);
		Font& operator=(Font&& other) noexcept;
		~Font();

		sf::Font& raw() noexcept;
		const sf::Font& raw() const noexcept;

	private:
		std::unique_ptr<sf::Font> m_font;
	};

}

#endif
