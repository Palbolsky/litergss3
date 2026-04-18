#ifndef CGSS_FONT_LOADER_H
#define CGSS_FONT_LOADER_H

#include <string>

namespace cgss {
	class Font;

	struct FontLoaderMethod {
		static bool load(cgss::Font& font, const std::string& filename);
	};
}

#endif
