#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_FONT_ATLAS_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_FONT_ATLAS_H

// raylib font atlas with SFML's text metrics.
//
// raylib's LoadFontEx sizes glyphs with stbtt_ScaleForPixelHeight: the
// requested size is the font's ascent - descent span. SFML (FreeType's
// FT_Set_Pixel_Sizes) makes it the em size instead. The two differ by the
// font's (ascent - descent) / unitsPerEm — 0.5 for PSDK's fonts, whose hhea
// descender is stored positive, so raylib drew them twice too big, upscaled
// from a coarse atlas. raylib's loader also takes an integer pixel height,
// which can't express em sizes exactly (13 px em = 6.5 px "height" there).
//
// This builder rasterizes each glyph at the em scale itself
// (stbtt_ScaleForMappingEmToPixels), binary alpha for crisp pixel fonts, and
// places glyphs so the first baseline is at y = characterSize, like sf::Text.
// The result is a regular raylib Font (baseSize = characterSize) drawn 1:1
// by DrawTextPro at that size.

#include <cstddef>

#include <raylib.h>

namespace cgss::backend::raylib_detail {

	// Builds `out` from TrueType/OpenType data. False when the data isn't a
	// usable font (`out` left untouched).
	bool buildFontAtlas(::Font& out, const unsigned char* data, int dataSize, int characterSize);

	// Glyphs on demand, as SFML loads them: the atlas starts with the common
	// ranges, and a text using characters the font has but the atlas lacks
	// (PSDK's number font draws digits at box-drawing code points) gets them
	// added, the atlas rebuilt in place. Fonts loaded through
	// loadFontAtlas only; forgetFontAtlas when the font goes away.
	bool loadFontAtlas(::Font& font, const unsigned char* data, int dataSize, int characterSize);
	void ensureFontGlyphs(::Font& font, const char* utf8, std::size_t size);
	void forgetFontAtlas(const ::Font& font);

}

#endif
