#include "Common/Profiling.h"
#include "LiteCGSS/Graphics/Font.h"
#include "LiteCGSS/Graphics/Serializers/FontLoader.h"

bool cgss::FontLoaderMethod::load(cgss::Font& font, const std::string& filename) {
	CGSS_PROFILE_ZONE("cgss::FontLoaderMethod::load");
	// The VFS route (and the bytes' lifetime) belongs to cgss::Font
	return font.load(filename);
}
