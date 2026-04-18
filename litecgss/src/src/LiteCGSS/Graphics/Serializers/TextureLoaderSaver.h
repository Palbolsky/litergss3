#ifndef CGSS_TEXTURE_LOADER_SAVER_H
#define CGSS_TEXTURE_LOADER_SAVER_H

#include "LiteCGSS/Common/Loader.h"
#include "LiteCGSS/Common/Saver.h"

namespace sf {
	class Texture;
	class Image;
}

namespace cgss {
	using TextureLoader = Loader<sf::Texture, sf::Image&>;
	using TextureSaver = Saver<sf::Texture>;
}

#endif
