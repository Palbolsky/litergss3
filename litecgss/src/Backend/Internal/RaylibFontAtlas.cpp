#include "Backend/Internal/RaylibFontAtlas.h"

#include <cmath>
#include <cstring>
#include <vector>

// Private copy: raylib embeds stb_truetype statically and doesn't export it.
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

namespace cgss::backend::raylib_detail {

	namespace {
		// Characters the atlas holds, when the font has them: ASCII, Latin-1,
		// Latin Extended-A, and the symbols game texts commonly use. A missing
		// codepoint is drawn as '?' by raylib.
		constexpr int kCodepointRanges[][2] = {
			{ 0x0020, 0x007E }, // ASCII
			{ 0x00A0, 0x017F }, // Latin-1 Supplement, Latin Extended-A
			{ 0x2010, 0x2027 }, // dashes, quotes, bullet, ellipsis
			{ 0x2030, 0x203A }, // per mille, primes, angle quotes
			{ 0x20AC, 0x20AC }, // euro
			{ 0x2122, 0x2122 }, // trade mark
			{ 0x2190, 0x2193 }, // arrows
			{ 0x2640, 0x2642 }, // female / male signs
			{ 0x266A, 0x266B }, // notes
		};

		// Coverage at or above this becomes opaque, the rest transparent
		// (raylib's FONT_BITMAP threshold): pixel fonts stay crisp.
		constexpr unsigned char kAlphaThreshold = 80;

		// Grayscale image for a glyph bitmap, allocated with raylib's
		// allocator (UnloadFont releases it). Empty glyphs (space...) get a
		// 1x1 transparent pixel: the atlas packer needs a non-empty image.
		::Image glyphImage(const unsigned char* bitmap, int w, int h) {
			if (bitmap == nullptr || w <= 0 || h <= 0) {
				auto* pixel = static_cast<unsigned char*>(MemAlloc(1));
				pixel[0] = 0;
				return ::Image{ pixel, 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE };
			}
			auto* pixels = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(w * h)));
			for (int i = 0; i < w * h; ++i) {
				pixels[i] = bitmap[i] >= kAlphaThreshold ? 255 : 0;
			}
			return ::Image{ pixels, w, h, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE };
		}
	}

	bool buildFontAtlas(::Font& out, const unsigned char* data, int dataSize, int characterSize) {
		if (data == nullptr || dataSize <= 0 || characterSize <= 0) return false;
		stbtt_fontinfo info;
		const int offset = stbtt_GetFontOffsetForIndex(data, 0);
		if (offset < 0 || !stbtt_InitFont(&info, data, offset)) return false;
		const float scale = stbtt_ScaleForMappingEmToPixels(&info, static_cast<float>(characterSize));

		std::vector<int> codepoints;
		for (const auto& range : kCodepointRanges) {
			for (int cp = range[0]; cp <= range[1]; ++cp) {
				// Space and '?' (raylib's fallback glyph) are always present.
				if (cp == ' ' || cp == '?' || stbtt_FindGlyphIndex(&info, cp) != 0) codepoints.push_back(cp);
			}
		}

		const int count = static_cast<int>(codepoints.size());
		auto* glyphs = static_cast<::GlyphInfo*>(MemAlloc(static_cast<unsigned int>(count) * sizeof(::GlyphInfo)));
		for (int i = 0; i < count; ++i) {
			const int cp = codepoints[static_cast<std::size_t>(i)];
			int w = 0, h = 0, xoff = 0, yoff = 0;
			unsigned char* bitmap = stbtt_GetCodepointBitmap(&info, scale, scale, cp, &w, &h, &xoff, &yoff);
			int advance = 0, leftBearing = 0;
			stbtt_GetCodepointHMetrics(&info, cp, &advance, &leftBearing);

			glyphs[i].value    = cp;
			glyphs[i].offsetX  = xoff;
			// yoff is the bitmap top relative to the baseline (negative above
			// it); sf::Text puts the first baseline at y = characterSize.
			glyphs[i].offsetY  = characterSize + yoff;
			glyphs[i].advanceX = static_cast<int>(std::lround(static_cast<float>(advance) * scale));
			glyphs[i].image    = glyphImage(bitmap, w, h);
			stbtt_FreeBitmap(bitmap, nullptr);
		}

		::Rectangle* recs = nullptr;
		// padding=4 like raylib's LoadFontEx: glyphs never touch in the atlas,
		// so sampling a glyph rect's edge doesn't bleed its neighbour in.
		::Image atlas = GenImageFontAtlas(glyphs, &recs, count, characterSize, /*padding*/ 4, /*packMethod*/ 0);

		::Font font{};
		font.baseSize     = characterSize;
		font.glyphCount   = count;
		font.glyphPadding = 0;
		font.glyphs       = glyphs;
		font.recs         = recs;
		font.texture      = LoadTextureFromImage(atlas);
		UnloadImage(atlas);
		if (font.texture.id == 0) {
			UnloadFont(font);
			return false;
		}
		SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
		out = font;
		return true;
	}

}
