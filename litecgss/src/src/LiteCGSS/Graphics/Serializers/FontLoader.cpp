#include <iostream>
#include <memory>
#include "LiteCGSS/Common/Assets.h"
#include "FontLoader.h"

static std::vector<std::unique_ptr<std::vector<char>>> gFonts;

bool cgss::FontLoaderMethod::load(sf::Font& font, const std::string& filename) {
#ifdef LITECGSS_USE_PHYSFS
	if (!AssetFile::exists(filename)) {
		return font.loadFromFile(filename.c_str());
	}
	try {
		gFonts.push_back(std::make_unique<std::vector<char>>(AssetFile { filename, "r" }.fullLoad()));
		auto& last = *gFonts.back();
		return font.loadFromMemory(&last[0], last.size());
	} catch (...) {
		std::cerr << "Error while loading Font from assets \"" << filename << "\"" << std::endl;
		return false;
	}
#else
	return font.loadFromFile(filename.c_str());
#endif
}
