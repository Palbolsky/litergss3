#include "Backend/Internal/RaylibFontAtlas.h"
#include "Views/GLContext.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
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

	namespace {
		bool initFontInfo(stbtt_fontinfo& info, const unsigned char* data, int dataSize) {
			if (data == nullptr || dataSize <= 0) return false;
			const int offset = stbtt_GetFontOffsetForIndex(data, 0);
			return offset >= 0 && stbtt_InitFont(&info, data, offset) != 0;
		}

		// The common ranges' characters the font has, plus space and '?'
		// (raylib's fallback glyph), always present.
		std::vector<int> initialCodepoints(const stbtt_fontinfo& info) {
			std::vector<int> codepoints;
			for (const auto& range : kCodepointRanges) {
				for (int cp = range[0]; cp <= range[1]; ++cp) {
					if (cp == ' ' || cp == '?' || stbtt_FindGlyphIndex(&info, cp) != 0) codepoints.push_back(cp);
				}
			}
			return codepoints;
		}

		bool buildAtlas(::Font& out, const stbtt_fontinfo& info, int characterSize, const std::vector<int>& codepoints);

		// What a loaded font was built from, to add glyphs later.
		struct FontSource {
			std::vector<unsigned char> data;
			stbtt_fontinfo info{};
			int characterSize = 0;
			std::unordered_set<int> baked;   // in the atlas
			std::unordered_set<int> missing; // asked for, not in the font
		};
		using FontSources = std::unordered_map<const ::Font*, std::unique_ptr<FontSource>>;
		// Never destroyed: a font held by a static of another file (LiteRGSS's
		// font table) is destroyed at exit, maybe after this file's statics,
		// and still forgets its atlas then (a destroyed map: double free).
		std::mutex& sourcesMutex() {
			static auto* mutex = new std::mutex;
			return *mutex;
		}
		FontSources& sources() {
			static auto* map = new FontSources;
			return *map;
		}
	}

	bool buildFontAtlas(::Font& out, const unsigned char* data, int dataSize, int characterSize) {
		stbtt_fontinfo info;
		if (characterSize <= 0 || !initFontInfo(info, data, dataSize)) return false;
		return buildAtlas(out, info, characterSize, initialCodepoints(info));
	}

	bool loadFontAtlas(::Font& font, const unsigned char* data, int dataSize, int characterSize) {
		cgss::detail::ensureGLContext();
		if (characterSize <= 0 || data == nullptr || dataSize <= 0) return false;
		auto source = std::make_unique<FontSource>();
		source->data.assign(data, data + dataSize); // stb reads it for the font's lifetime
		source->characterSize = characterSize;
		if (!initFontInfo(source->info, source->data.data(), dataSize)) return false;
		const auto codepoints = initialCodepoints(source->info);
		::Font built{};
		if (!buildAtlas(built, source->info, characterSize, codepoints)) return false;
		forgetFontAtlas(font);
		if (font.texture.id != 0) UnloadFont(font);
		font = built;
		source->baked.insert(codepoints.begin(), codepoints.end());
		std::lock_guard<std::mutex> lock(sourcesMutex());
		sources()[&font] = std::move(source);
		return true;
	}

	// Whether the text has characters the font has and its atlas lacks
	static bool missingFontGlyphs(const ::Font& font, const char* utf8, std::size_t size) {
		std::lock_guard<std::mutex> lock(sourcesMutex());
		const auto it = sources().find(&font);
		if (it == sources().end() || utf8 == nullptr) return false;
		const FontSource& source = *it->second;
		const char* p = utf8;
		const char* end = utf8 + size;
		while (p < end) {
			int bytes = 0;
			const int cp = GetCodepointNext(p, &bytes);
			p += bytes > 0 ? bytes : 1;
			if (cp < 0x20 || source.baked.count(cp) != 0 || source.missing.count(cp) != 0) continue;
			return true;
		}
		return false;
	}

	void ensureFontGlyphs(::Font& font, const char* utf8, std::size_t size) {
		// The atlas grows in GL: a text whose glyphs are all there doesn't
		// take the context (a thread with no GL work would wait for a frame
		// for nothing). The context comes first, the sources lock second,
		// as everywhere else: missing glyphs, the scan runs again with both.
		if (!missingFontGlyphs(font, utf8, size)) return;
		cgss::detail::ensureGLContext();
		std::lock_guard<std::mutex> lock(sourcesMutex());
		const auto it = sources().find(&font);
		if (it == sources().end() || utf8 == nullptr) return;
		FontSource& source = *it->second;
		std::vector<int> added;
		const char* p = utf8;
		const char* end = utf8 + size;
		while (p < end) {
			int bytes = 0;
			const int cp = GetCodepointNext(p, &bytes);
			p += bytes > 0 ? bytes : 1;
			if (cp < 0x20 || source.baked.count(cp) != 0 || source.missing.count(cp) != 0) continue;
			if (stbtt_FindGlyphIndex(&source.info, cp) == 0) {
				source.missing.insert(cp);
				continue;
			}
			if (std::find(added.begin(), added.end(), cp) == added.end()) added.push_back(cp);
		}
		if (added.empty()) return;
		std::vector<int> codepoints(source.baked.begin(), source.baked.end());
		codepoints.insert(codepoints.end(), added.begin(), added.end());
		std::sort(codepoints.begin(), codepoints.end());
		::Font built{};
		if (!buildAtlas(built, source.info, source.characterSize, codepoints)) return;
		if (font.texture.id != 0) UnloadFont(font);
		font = built;
		source.baked.insert(added.begin(), added.end());
	}

	void forgetFontAtlas(const ::Font& font) {
		std::lock_guard<std::mutex> lock(sourcesMutex());
		sources().erase(&font);
	}

	namespace {
	bool buildAtlas(::Font& out, const stbtt_fontinfo& info, int characterSize, const std::vector<int>& codepoints) {
		const float scale = stbtt_ScaleForMappingEmToPixels(&info, static_cast<float>(characterSize));

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

}
