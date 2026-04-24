#ifndef CGSS_BACKEND_RAYLIB_SCROLL_TEXT_H
#define CGSS_BACKEND_RAYLIB_SCROLL_TEXT_H

// Raylib-native cgss::ScrollText — parallel implementation to the SFML
// version in src/LiteCGSS/Graphics/ScrollText.{h,cpp}. Selected by CMake
// when CGSS_BACKEND=raylib (Backend/Specializations/raylib/ is globbed
// only for that backend; the SFML ScrollText is filtered out of the
// engine's private-sources list under non-sfml backends).
//
// Consumer: src/LiteCGSS/Backend/Specializations/raylib/Text.cpp. The
// public Text.h forward-declares `cgss::ScrollText`; each backend supplies
// its complete type here (or in Graphics/ for SFML). The two impls share
// a name and nothing else — their internal APIs can diverge freely since
// they're only ever called from the matching backend's Text.cpp.
//
// Phase 1 scope: DrawTextPro-based rendering. Supports string, font,
// character size, fill color, position/origin/scale, numCharsToDraw
// (typewriter), and smoothing.
//
// Phase 2 adds: outline (8-tap stamp — raylib has no SFML-style per-glyph
// outline render), drop shadow (3-stamp bottom-right, matches the SFML
// vendored impl), Underlined and StrikeThrough style flags (filled rects
// using heuristic line metrics — raylib exposes no underlinePosition /
// underlineThickness), and synthetic Bold (1-offset stamp).
//
// Still deferred: Italic (would need a custom rlMultMatrixf shear matrix
// post-view — see the SFML italicShear constant of 0.209f for reference);
// kerning (raylib has no GetKerning equivalent); true per-glyph bounds
// via GetGlyphInfo / GetGlyphAtlasRec (Phase 3).

#include <raylib.h>

#include <cstdint>
#include <limits>
#include <string>

namespace cgss {

	class ScrollText {
	public:
		// Style bit values — kept in lockstep with the SFML ScrollText
		// enum (src/LiteCGSS/Graphics/ScrollText.h) so consumers can pass
		// the same uint32 mask regardless of backend.
		enum Style : std::uint32_t {
			Regular       = 0,
			Bold          = 1u << 0,
			Italic        = 1u << 1,
			Underlined    = 1u << 2,
			StrikeThrough = 1u << 3,
		};

		ScrollText() = default;

		void setString(const std::string& utf8);
		void setFont(const ::Font& font);
		void setCharacterSize(unsigned int size);
		void setFillColor(::Color c);
		void setOutlineColor(::Color c);
		void setOutlineThickness(float thickness);
		void setDrawShadow(bool on);
		void setStyle(std::uint32_t style);
		void setSmooth(bool smooth);
		void setLineHeight(float h);
		void setLetterSpacing(float factor);
		void setLineSpacing(float factor);
		void setNumCharToDraw(std::uint32_t n);

		void setPosition(float x, float y);
		void setOrigin(float ox, float oy);
		void setScale(float sx, float sy);

		const ::Color& getFillColor() const noexcept { return m_fill; }
		const ::Color& getOutlineColor() const noexcept { return m_outline; }
		float getOutlineThickness() const noexcept { return m_outlineThickness; }
		bool getDrawShadow() const noexcept { return m_drawShadow; }
		bool getSmooth() const noexcept { return m_smooth; }
		std::uint32_t getStyle() const noexcept { return m_style; }
		unsigned int getCharacterSize() const noexcept { return m_characterSize; }
		std::uint32_t getNumCharToDraw() const noexcept { return m_numCharsToDraw; }

		// Returns {0, 0, width, height} — the text's measured box in local
		// (pre-transform) coordinates. Mirrors sf::Text::getLocalBounds()
		// in semantics used by Text.cpp's updateAlign.
		::Rectangle getLocalBounds() const;
		std::uint32_t getTextWidth(const std::string& utf8) const;

		// Issues raylib draw calls against the current active render state
		// (whichever BeginDrawing / BeginTextureMode is in scope). Raylib
		// has no per-call target binding — matches the pattern used by
		// RaylibBackend::Ops::draw_sprite / draw_shape.
		void draw() const;

	private:
		std::string m_utf8;
		const ::Font* m_font = nullptr;
		unsigned int m_characterSize = 30;
		float m_letterSpacingFactor = 1.f;
		float m_lineSpacingFactor = 1.f;
		float m_lineHeight = 16.f;
		std::uint32_t m_style = 0;
		::Color m_fill { 255, 255, 255, 255 };
		::Color m_outline { 0, 0, 0, 255 };
		float m_outlineThickness = 0.f;
		bool m_drawShadow = true;
		// Default smooth=true matches sf::Font's own default (SFML text comes
		// out bilinear-filtered unless a consumer opts out). Raylib bakes its
		// atlas at a fixed base size (32 px by default in font_load_from_file)
		// and DrawTextPro rescales via texture sampling — POINT filtering on
		// the common downscale path (e.g. 32-atlas → 18 render) aliases glyph
		// edges and reads as slightly thinner / smaller to the eye. BILINEAR
		// is the closer match to sf::Text's native-rasterized look.
		bool m_smooth = true;
		std::uint32_t m_numCharsToDraw = (std::numeric_limits<std::uint32_t>::max)();
		float m_x = 0.f, m_y = 0.f;
		float m_ox = 0.f, m_oy = 0.f;
		float m_sx = 1.f, m_sy = 1.f;
	};

}

#endif
