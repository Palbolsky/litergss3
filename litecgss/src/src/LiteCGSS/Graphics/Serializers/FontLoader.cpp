#include <iostream>
#include <memory>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Graphics/Font.h"
#include "FontLoader.h"

static std::vector<std::unique_ptr<std::vector<char>>> gFonts;

bool cgss::FontLoaderMethod::load(cgss::Font& font, const std::string& filename) {
	using Ops = cgss::backend::ActiveBackend::Ops;
	auto& raw = cgss::backend::native_of(font);
#ifdef LITECGSS_USE_PHYSFS
	if (!AssetFile::exists(filename)) {
		return Ops::font_load_from_file(raw, filename.c_str());
	}
	try {
		gFonts.push_back(std::make_unique<std::vector<char>>(AssetFile { filename, "r" }.fullLoad()));
		auto& last = *gFonts.back();
		return Ops::font_load_from_memory(raw, &last[0], last.size());
	} catch (...) {
		std::cerr << "Error while loading Font from assets \"" << filename << "\"" << std::endl;
		return false;
	}
#else
	return Ops::font_load_from_file(raw, filename.c_str());
#endif
}
