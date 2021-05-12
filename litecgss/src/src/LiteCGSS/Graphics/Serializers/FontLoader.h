#ifndef CGSS_FONT_LOADER_H
#define CGSS_FONT_LOADER_H

#include <string>
#include <SFML/Graphics/Font.hpp>

namespace cgss {
	struct FontLoaderMethod {
		static bool load(sf::Font& font, const std::string& filename);
	};
}

#endif