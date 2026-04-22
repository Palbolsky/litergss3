// FontLoader's body uses sf::Font::loadFromFile / loadFromMemory directly.
// Templating it (and the matching backend op family) is out of scope for
// the current PR M continuation; guard the TU so non-SFML backends compile.
#ifdef CGSS_BACKEND_SFML

#include <iostream>
#include <memory>
#include <SFML/Graphics/Font.hpp>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Graphics/Font.h"
#include "FontLoader.h"

static std::vector<std::unique_ptr<std::vector<char>>> gFonts;

bool cgss::FontLoaderMethod::load(cgss::Font& font, const std::string& filename) {
	sf::Font& raw = cgss::backend::native_of(font);
#ifdef LITECGSS_USE_PHYSFS
	if (!AssetFile::exists(filename)) {
		return raw.loadFromFile(filename.c_str());
	}
	try {
		gFonts.push_back(std::make_unique<std::vector<char>>(AssetFile { filename, "r" }.fullLoad()));
		auto& last = *gFonts.back();
		return raw.loadFromMemory(&last[0], last.size());
	} catch (...) {
		std::cerr << "Error while loading Font from assets \"" << filename << "\"" << std::endl;
		return false;
	}
#else
	return raw.loadFromFile(filename.c_str());
#endif
}

#endif  // CGSS_BACKEND_SFML
