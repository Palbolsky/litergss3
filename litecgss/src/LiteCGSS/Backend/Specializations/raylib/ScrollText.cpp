#include "LiteCGSS/Backend/Specializations/raylib/ScrollText.h"

#include <rlgl.h>

#include <cmath>

#include "LiteCGSS/Backend/ActiveBackend.h"

namespace {

	// Returns the number of bytes in `utf8` that encode the first `n`
	// codepoints, or the whole string's length if it has fewer than `n`
	// codepoints. Uses raylib's UTF-8 iterator so whatever subset raylib
	// understands is what we count.
	std::size_t utf8BytesForCodepoints(const std::string& utf8, std::uint32_t n) {
		if (n == 0 || utf8.empty()) return 0;
		const char* begin = utf8.c_str();
		const char* p     = begin;
		const char* end   = begin + utf8.size();
		std::uint32_t seen = 0;
		while (p < end && seen < n) {
			int size = 0;
			::GetCodepointNext(p, &size);
			if (size <= 0) break;
			p += size;
			++seen;
		}
		return static_cast<std::size_t>(p - begin);
	}

	float fontSizeF(unsigned int charSize) noexcept {
		return static_cast<float>(charSize);
	}

	// Raylib's DrawTextPro takes a per-character pixel spacing. SFML's
	// letterSpacing is a multiplier on the space glyph's width. Phase 1
	// approximates: 0 spacing at factor 1.0, +1 px per unit factor over 1.0.
	// Good enough for the RPG-UI use case; Phase 2 may route per-glyph.
	float spacingFromFactor(float factor) noexcept {
		return std::max(0.f, (factor - 1.f) * 2.f);
	}

}

void cgss::ScrollText::setString(const std::string& utf8) {
	m_utf8 = utf8;
}

void cgss::ScrollText::setFont(const ::Font& font) {
	m_font = &font;
	if (m_font->texture.id != 0) {
		::SetTextureFilter(m_font->texture,
		                   m_smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
	}
}

void cgss::ScrollText::setCharacterSize(unsigned int size) {
	m_characterSize = size;
}

void cgss::ScrollText::setFillColor(::Color c) {
	m_fill = c;
}

void cgss::ScrollText::setOutlineColor(::Color c) {
	m_outline = c;
}

void cgss::ScrollText::setOutlineThickness(float thickness) {
	m_outlineThickness = thickness;
}

void cgss::ScrollText::setDrawShadow(bool on) {
	m_drawShadow = on;
}

void cgss::ScrollText::setStyle(std::uint32_t style) {
	m_style = style;
}

void cgss::ScrollText::setSmooth(bool smooth) {
	m_smooth = smooth;
	if (m_font && m_font->texture.id != 0) {
		::SetTextureFilter(m_font->texture,
		                   smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
	}
}

void cgss::ScrollText::setLineHeight(float h) {
	m_lineHeight = h;
}

void cgss::ScrollText::setLetterSpacing(float factor) {
	m_letterSpacingFactor = factor;
}

void cgss::ScrollText::setLineSpacing(float factor) {
	m_lineSpacingFactor = factor;
}

void cgss::ScrollText::setNumCharToDraw(std::uint32_t n) {
	m_numCharsToDraw = n;
}

void cgss::ScrollText::setPosition(float x, float y) {
	m_x = x;
	m_y = y;
}

void cgss::ScrollText::setOrigin(float ox, float oy) {
	m_ox = ox;
	m_oy = oy;
}

void cgss::ScrollText::setScale(float sx, float sy) {
	m_sx = sx;
	m_sy = sy;
}

::Rectangle cgss::ScrollText::getLocalBounds() const {
	if (!m_font || m_utf8.empty()) return ::Rectangle{ 0.f, 0.f, 0.f, 0.f };
	const ::Vector2 size = ::MeasureTextEx(*m_font, m_utf8.c_str(),
	                                       fontSizeF(m_characterSize),
	                                       spacingFromFactor(m_letterSpacingFactor));
	// Matches SFML ScrollText: outline expands the bounding box by ceil(thickness)
	// in every direction so layout code (centering, clipping) accounts for the
	// stamped pixels around the glyph fill.
	if (m_outlineThickness != 0.f && !m_drawShadow) {
		const float outline = std::fabs(std::ceil(m_outlineThickness));
		return ::Rectangle{ -outline, -outline, size.x + 2.f * outline, size.y + 2.f * outline };
	}
	return ::Rectangle{ 0.f, 0.f, size.x, size.y };
}

std::uint32_t cgss::ScrollText::getTextWidth(const std::string& utf8) const {
	if (!m_font || utf8.empty()) return 0;
	const ::Vector2 size = ::MeasureTextEx(*m_font, utf8.c_str(),
	                                       fontSizeF(m_characterSize),
	                                       spacingFromFactor(m_letterSpacingFactor));
	return static_cast<std::uint32_t>(size.x);
}

void cgss::ScrollText::draw() const {
	if (!m_font || m_font->texture.id == 0 || m_utf8.empty() || m_numCharsToDraw == 0) {
		return;
	}

	const std::size_t bytes = utf8BytesForCodepoints(m_utf8, m_numCharsToDraw);
	if (bytes == 0) return;

	// Null-terminate the truncated view. `substr` allocates; acceptable
	// for UI text sizes (< a few hundred chars). Phase 2 can amortize by
	// caching the truncated buffer when m_utf8 / m_numCharsToDraw change.
	const std::string truncated = m_utf8.substr(0, bytes);
	const char* text    = truncated.c_str();
	const float fs      = fontSizeF(m_characterSize);
	const float spacing = spacingFromFactor(m_letterSpacingFactor);

	const bool drawBold       = (m_style & ScrollText::Bold) != 0;
	const bool drawUnderlined = (m_style & ScrollText::Underlined) != 0;
	const bool drawStrike     = (m_style & ScrollText::StrikeThrough) != 0;

	// Compose the currently-installed raylib View with the user scale so
	// text inside a Viewport / FramedView renders at the view-mapped screen
	// pixels and stays inside the view's scissor region. Mirrors the
	// derive_current_view path used by draw_sprite_impl / draw_shape —
	// without this, DrawTextPro paints at raw screen coords and vanishes
	// under a view-installed scissor (e.g. text inside a FramedView).
	// DrawTextPro accepts no scale parameter, so the rlgl matrix is the
	// cleanest common path for both non-uniform scale and view zoom.
	const auto vd = cgss::backend::ActiveBackend::Ops::derive_current_view();

	::rlPushMatrix();
	::rlTranslatef(vd.tx + m_x * vd.zoomX, vd.ty + m_y * vd.zoomY, 0.f);
	::rlScalef(m_sx * vd.zoomX, m_sy * vd.zoomY, 1.f);

	const ::Vector2 origin = { m_ox, m_oy };
	const float rot        = vd.rotation;

	// Shadow vs outline — matches SFML ScrollText's exclusivity: shadow
	// short-circuits the outline pass. Shadow renders as 3 stamps in the
	// bottom-right direction (mirroring addGlyphQuad's +1/+1/+1,+1/0,+1
	// triplet in the SFML vendored impl). Outline stamps 8 directions at
	// m_outlineThickness; raylib has no per-glyph outline equivalent, so
	// this approximates via repeated DrawTextPro calls in outline color.
	if (m_drawShadow) {
		::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 0.f }, origin, rot, fs, spacing, m_outline);
		::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 1.f }, origin, rot, fs, spacing, m_outline);
		::DrawTextPro(*m_font, text, ::Vector2{ 0.f, 1.f }, origin, rot, fs, spacing, m_outline);
	} else if (m_outlineThickness > 0.f) {
		const float t = m_outlineThickness;
		const ::Vector2 offsets[8] = {
			{  t,  0.f }, { -t,  0.f },
			{ 0.f,  t  }, { 0.f, -t  },
			{  t,   t  }, { -t,  -t  },
			{  t,  -t  }, { -t,   t  },
		};
		for (const auto& off : offsets) {
			::DrawTextPro(*m_font, text, off, origin, rot, fs, spacing, m_outline);
		}
	}

	// Fill pass. Synthetic bold re-stamps at +1px — raylib has no synthetic
	// bold, and proper bold requires a separately-loaded font variant; this
	// thickens the stroke as a best-effort fallback.
	::DrawTextPro(*m_font, text, ::Vector2{ 0.f, 0.f }, origin, rot, fs, spacing, m_fill);
	if (drawBold) {
		::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 0.f }, origin, rot, fs, spacing, m_fill);
	}

	// Underline / StrikeThrough — raylib has no underline metrics API; pick
	// heuristic offsets that match typical typographic positions:
	//   underline baseline ~ 95% of character height,
	//   strikethrough      ~ 55% (mid-height).
	// Thickness = max(1, fs/16). Single-line extent only: MeasureTextEx on a
	// multi-line string returns the widest line, so strike/underline will
	// over-extend on multi-line text — tracked as a multi-line layout TODO.
	if (drawUnderlined || drawStrike) {
		const ::Vector2 extent = ::MeasureTextEx(*m_font, text, fs, spacing);
		const float thickness  = std::max(1.f, fs / 16.f);
		// Glyph top-left in matrix-local coords is (-ox, -oy); rectangles
		// line up with the text by offsetting likewise.
		const float left = -m_ox;
		if (drawUnderlined) {
			const float yTop = fs * 0.95f - m_oy - thickness * 0.5f;
			::DrawRectangleRec(::Rectangle{ left, yTop, extent.x, thickness }, m_fill);
		}
		if (drawStrike) {
			const float yTop = fs * 0.55f - m_oy - thickness * 0.5f;
			::DrawRectangleRec(::Rectangle{ left, yTop, extent.x, thickness }, m_fill);
		}
	}

	::rlPopMatrix();
}
