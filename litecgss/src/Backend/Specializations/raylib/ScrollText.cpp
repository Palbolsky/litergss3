#include "Backend/Specializations/raylib/ScrollText.h"
#include "Common/Profiling.h"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <string_view>

#include "Backend/ActiveBackend.h"

namespace {

	// Encased outline shader. SFML bakes the outline into a wider glyph
	// (font.getGlyph(charSize, bold, outlineThickness) returns a glyph rect
	// already containing the stroke around the fill); raylib has no
	// equivalent, and the pre-shader implementation collapsed the outline
	// to a single +1,+1 drop stamp because multi-direction stamps overlap
	// between adjacent letters at PSDK's tight letter spacings.
	//
	// This fragment shader recovers the SFML look without atlas
	// duplication: it samples the font atlas at +/-thickness in the 8
	// surrounding directions and emits outline color where any neighbour
	// has alpha but the centre does not — i.e. one-pixel inflation of the
	// glyph mask, restricted to the rendered quad. To make outline pixels
	// land OUTSIDE the glyph's tight bounds (where SFML's baked outline
	// lives), the outline-pass DrawTextPro is given a temporary Font with
	// glyphPadding > 0. The atlas was built with GenImageFontAtlas
	// padding=4, so up to 4 px of expansion is safe: the source-rect
	// expansion samples into pre-zeroed inter-glyph gutter, never into
	// neighbouring glyphs.
	constexpr const char* kOutlineFragShader = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 atlasTexel;
uniform float thickness;
out vec4 finalColor;
void main() {
    float centre = texture(texture0, fragTexCoord).a;
    float n = 0.0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            vec2 off = vec2(float(dx), float(dy)) * atlasTexel * thickness;
            n = max(n, texture(texture0, fragTexCoord + off).a);
        }
    }
    float outline = n * (1.0 - centre);
    vec4 base = fragColor * colDiffuse;
    finalColor = vec4(base.rgb, base.a * outline);
}
)GLSL";

	struct OutlineShaderState {
		::Shader shader{};
		int locAtlasTexel = -1;
		int locThickness  = -1;
		bool tried        = false;
	};

	OutlineShaderState& outlineShaderState() {
		static OutlineShaderState s;
		return s;
	}

	// Lazy-load on first use. Returns true only when the program is bound
	// and uniform locations resolved. The Shader is intentionally leaked:
	// raylib's CloseWindow runs as an atexit handler in
	// RaylibBackend::window_initialize, so the GL context outlives us;
	// calling UnloadShader from a static destructor would race with
	// context teardown.
	bool ensureOutlineShader() {
		auto& s = outlineShaderState();
		if (s.tried) return s.shader.id != 0;
		s.tried = true;
		s.shader = ::LoadShaderFromMemory(nullptr, kOutlineFragShader);
		if (s.shader.id == 0) return false;
		s.locAtlasTexel = ::GetShaderLocation(s.shader, "atlasTexel");
		s.locThickness  = ::GetShaderLocation(s.shader, "thickness");
		return true;
	}

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

	// Raylib / stb_truetype treats the `fontSize` parameter as the
	// ascender-to-descender span (via stbtt_ScaleForPixelHeight). SFML /
	// FreeType treats `characterSize` as the em-size (FT_Set_Pixel_Sizes).
	// For typical Latin TTFs em ≈ 0.78-0.82 × (ascent − descent), so a
	// same-numeric fontSize between the two backends renders glyphs ~22%
	// smaller on raylib. Compensate by inflating the value we hand to
	// DrawTextPro / MeasureTextEx.
	//
	// Picked at 1.25 — matches DejaVu, Liberation Sans, Noto Sans, and the
	// majority of bundled Linux system fonts within ~3%. Fonts with wider
	// or tighter metrics (e.g. Arial is closer to 1.12, some CJK fonts go
	// above 1.3) will still look off; a Phase-3 refinement could compute
	// this ratio at setFont() time from a reference glyph's atlas rect.
	// Earlier revisions multiplied by 1.25 to roughly match SFML's perceived
	// glyph height. That extra factor pushes pixel fonts off integer
	// boundaries when the surrounding rlScalef applies a window-scale
	// multiplier on top, causing the notched look PSDK exhibits at
	// title-menu sizes. Drop the compensation: render at exactly the
	// requested character size so pixel fonts stay on integer pixel grid.
	constexpr float kRaylibEmCompensation = 1.0f;

	float fontSizeF(unsigned int charSize) noexcept {
		return static_cast<float>(charSize) * kRaylibEmCompensation;
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

void cgss::ScrollText::setFillColor(cgss::Color c) {
	m_fill = ::Color{ c.r, c.g, c.b, c.a };
}

void cgss::ScrollText::setOutlineColor(cgss::Color c) {
	m_outline = ::Color{ c.r, c.g, c.b, c.a };
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

cgss::FloatRect cgss::ScrollText::getLocalBounds() const {
	if (!m_font || m_utf8.empty() || m_font->baseSize <= 0) return cgss::FloatRect{ 0.f, 0.f, 0.f, 0.f };
	// Same bounds as SFML's ScrollText: the glyphs' ink extents (not their
	// advances — the last glyph's right bearing doesn't count), whitespace
	// advancing the max, the outline expanding them. Text alignment relies on
	// this width. Advances are the ones DrawTextPro uses.
	const float fs      = fontSizeF(m_characterSize);
	const float scale   = fs / static_cast<float>(m_font->baseSize);
	const float spacing = spacingFromFactor(m_letterSpacingFactor);
	float x = 0.f, y = 0.f;
	float minX = fs, minY = fs, maxX = 0.f, maxY = 0.f;
	const char* text = m_utf8.c_str();
	const char* end  = text + m_utf8.size();
	while (text < end) {
		int bytes = 0;
		const int cp = ::GetCodepointNext(text, &bytes);
		text += bytes > 0 ? bytes : 1;
		const int index = ::GetGlyphIndex(*m_font, cp);
		const float advance = static_cast<float>(m_font->glyphs[index].advanceX) * scale + spacing;
		if (cp == ' ' || cp == '\t' || cp == '\n') {
			minX = std::min(minX, x);
			minY = std::min(minY, y);
			if (cp == ' ') x += advance;
			else if (cp == '\t') x += advance * 4.f;
			else { y += fs; x = 0.f; }
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
			continue;
		}
		const ::GlyphInfo& glyph = m_font->glyphs[index];
		const ::Rectangle& rec   = m_font->recs[index];
		const float left = x + static_cast<float>(glyph.offsetX) * scale;
		const float top  = y + static_cast<float>(glyph.offsetY) * scale;
		minX = std::min(minX, left);
		maxX = std::max(maxX, left + rec.width * scale);
		minY = std::min(minY, top);
		maxY = std::max(maxY, top + rec.height * scale);
		x += advance;
	}
	if (m_outlineThickness != 0.f) {
		const float outline = std::fabs(std::ceil(m_outlineThickness));
		minX -= outline;
		maxX += outline;
		minY -= outline;
		maxY += outline;
	}
	return cgss::FloatRect{ minX, minY, maxX - minX, maxY - minY };
}

std::uint32_t cgss::ScrollText::getTextWidth(const std::string& utf8) const {
	if (!m_font || utf8.empty()) return 0;
	const ::Vector2 size = ::MeasureTextEx(*m_font, utf8.c_str(),
	                                       fontSizeF(m_characterSize),
	                                       spacingFromFactor(m_letterSpacingFactor));
	return static_cast<std::uint32_t>(size.x);
}

void cgss::ScrollText::draw() const {
	CGSS_PROFILE_ZONE("cgss::ScrollText::draw");
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
	const bool drawItalic     = (m_style & ScrollText::Italic) != 0;
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

	// Synthetic italic — column-major 4x4 with x-axis shear of `italicShear`
	// matching SFML's vendored ScrollText constant (0.209 ≈ tan(11.5°)). The
	// shear is applied AFTER scale in the matrix stack so it produces the
	// same slanted-fill look at any view zoom; the `-m_oy * shear` translate
	// re-anchors the origin row so glyphs above the origin slant the same
	// distance as glyphs below it (otherwise origin-relative text would drift
	// horizontally as font size changed).
	if (drawItalic) {
		constexpr float italicShear = 0.209f;
		const float shearMatrix[16] = {
			1.f,         0.f, 0.f, 0.f,
			italicShear, 1.f, 0.f, 0.f,
			0.f,         0.f, 1.f, 0.f,
			0.f,         0.f, 0.f, 1.f,
		};
		::rlMultMatrixf(shearMatrix);
		::rlTranslatef(-m_oy * italicShear, 0.f, 0.f);
	}

	const ::Vector2 origin = { m_ox, m_oy };
	const float rot        = vd.rotation;

	// Shadow vs outline — shadow short-circuits the outline pass. Same
	// 3-stamp shadow as SFML's ScrollText: (+1, 0), (+1, +1), (0, +1). Glyphs
	// are binary-alpha and drawn 1:1 (Internal/RaylibFontAtlas), so the
	// integer stamps stay exact under any rlScalef.
	if (m_drawShadow) {
		::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 0.f }, origin, rot, fs, spacing, m_outline);
		::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 1.f }, origin, rot, fs, spacing, m_outline);
		::DrawTextPro(*m_font, text, ::Vector2{ 0.f, 1.f }, origin, rot, fs, spacing, m_outline);
	} else if (m_outlineThickness > 0.f) {
		// Encased outline via fragment shader. Falls back to the legacy
		// single-stamp +1,+1 if the shader fails to compile (e.g. a Mesa
		// driver that rejects #version 330 — unlikely on the GL 3.3
		// raylib already requires, but the fallback keeps PSDK readable
		// rather than going outline-less).
		if (ensureOutlineShader()) {
			const auto& s = outlineShaderState();
			const ::Vector2 atlasTexel = {
				1.0f / static_cast<float>(m_font->texture.width),
				1.0f / static_cast<float>(m_font->texture.height)
			};
			::SetShaderValue(s.shader, s.locAtlasTexel, &atlasTexel, SHADER_UNIFORM_VEC2);
			::SetShaderValue(s.shader, s.locThickness,  &m_outlineThickness, SHADER_UNIFORM_FLOAT);

			// font_load_from_file_with_size leaves Font::glyphPadding at 0,
			// which makes DrawTextPro render glyph quads tight to the
			// glyph's bitmap bounds — leaving no room for outline pixels
			// outside the glyph (where SFML's baked outline lives). Pass a
			// temporary copy with glyphPadding = ceil(thickness), clamped
			// to the GenImageFontAtlas inter-glyph padding (4 px) so the
			// expanded source rect samples the pre-zeroed gutter, never
			// into neighbouring glyphs.
			::Font fontCopy = *m_font;
			const int padding = std::clamp(
				static_cast<int>(std::ceil(m_outlineThickness)), 1, 4);
			fontCopy.glyphPadding = padding;

			::BeginShaderMode(s.shader);
			::DrawTextPro(fontCopy, text, ::Vector2{ 0.f, 0.f }, origin, rot, fs, spacing, m_outline);
			::EndShaderMode();
		} else {
			::DrawTextPro(*m_font, text, ::Vector2{ 1.f, 1.f }, origin, rot, fs, spacing, m_outline);
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
	// Thickness = max(1, fs/16). Per-line: split on '\n' and measure each
	// line's width independently — `MeasureTextEx` on a multi-line string
	// returns the widest line, so a single rect would over-extend on
	// shorter lines. raylib's DrawTextPro lays multi-line text top-down
	// stepping by the same fontSize used for measurement, so we match that
	// step here (raylib's GetFontDefault uses height==fontSize for the
	// baseline-to-baseline distance — close enough for our heuristic).
	if (drawUnderlined || drawStrike) {
		const float thickness = std::max(1.f, fs / 16.f);
		const float left      = -m_ox;
		const std::string_view textView{ truncated };
		std::size_t start = 0;
		std::size_t lineIndex = 0;
		while (start <= textView.size()) {
			const std::size_t nl = textView.find('\n', start);
			const std::size_t end = (nl == std::string_view::npos) ? textView.size() : nl;
			// MeasureTextEx accepts a NUL-terminated cstring; substr+c_str
			// is acceptable for UI text sizes (handful of lines).
			const std::string lineCopy(textView.substr(start, end - start));
			const ::Vector2 extent = lineCopy.empty()
				? ::Vector2{ 0.f, 0.f }
				: ::MeasureTextEx(*m_font, lineCopy.c_str(), fs, spacing);
			const float yLineTop = fs * static_cast<float>(lineIndex) - m_oy;
			if (drawUnderlined && extent.x > 0.f) {
				::DrawRectangleRec(::Rectangle{ left, yLineTop + fs * 0.95f - thickness * 0.5f,
				                                extent.x, thickness }, m_fill);
			}
			if (drawStrike && extent.x > 0.f) {
				::DrawRectangleRec(::Rectangle{ left, yLineTop + fs * 0.55f - thickness * 0.5f,
				                                extent.x, thickness }, m_fill);
			}
			if (nl == std::string_view::npos) break;
			start = nl + 1;
			++lineIndex;
		}
	}

	::rlPopMatrix();
}
