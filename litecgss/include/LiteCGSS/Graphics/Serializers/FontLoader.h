#ifndef CGSS_FONT_LOADER_H
#define CGSS_FONT_LOADER_H

#include <string>

#include "LiteCGSS/Graphics/FontFwd.h"

namespace cgss {

	struct FontLoaderMethod {
		static bool load(cgss::Font& font, const std::string& filename);
	};
}

#endif
