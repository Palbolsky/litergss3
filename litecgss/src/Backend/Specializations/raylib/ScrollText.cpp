#include "Backend/Specializations/raylib/ScrollText.h"
#include "Common/Profiling.h"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <string>
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
	//
	// The body is GLSL 330 and GLSL ES 3.00 alike: its #version (and the ES
	// precision) follows the shader target, as the preprocessed shaders do.
	// A desktop-only #version 330 failed on GL ES 3 (Android), silently
	// leaving the +1,+1 stamp below: outlines drawn as a drop shadow.
	constexpr const char* kOutlineFragShaderBody = R"GLSL(
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 atlasTexel;
uniform float thickness;
out vec4 finalColor;
void main() {
    float centre = texture(texture0, fragTexCoord).a;
    // How much of this pixel the outline's stroke covers, from each glyph
    // pixel around it (up to the atlas' 4 px glyph padding): the distance from
    // this pixel's centre to that glyph pixel's edge. Fitted on LiteRGSS2
    // (FreeType's round stroker): a pixel beside the glyph is covered, one it
    // touches diagonally only is ~70% (tests/scenes/text_shadow_outline.rb).
    float n = 0.0;
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            if (dx == 0 && dy == 0) continue;
            float d = length(max(abs(vec2(float(dx), float(dy))) - 0.5, 0.0));
            float cover = clamp((thickness + 0.5 - d) * 1.45 - 0.45, 0.0, 1.0);
            if (cover <= 0.0) continue;
            vec2 off = vec2(float(dx), float(dy)) * atlasTexel;
            n = max(n, cover * texture(texture0, fragTexCoord + off).a);
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
		using Ops = cgss::backend::ActiveBackend::Ops;
		const bool es = Ops::shader_preprocessor_target() == cgss::ShaderPreprocessor::Target::RaylibGlslEs300;
		const std::string source = std::string(es ? "#version 300 es\nprecision highp float;\n" : "#version 330\n")
			+ kOutlineFragShaderBody;
		s.shader = ::LoadShaderFromMemory(nullptr, source.c_str());
		if (s.shader.id == 0) {
			::TraceLog(LOG_WARNING, "LiteCGSS: the text outline shader failed to load: outlines are drawn as a +1,+1 stamp");
			return false;
		}
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

	// Baseline-to-baseline step between lines, in whole pixels: the text's
	// line height times its line spacing, as SFML's text steps (raylib's
	// own step is a fixed 15 px).
	float lineStepOf(float lineHeight, float spacingFactor) noexcept {
		return std::round(lineHeight * spacingFactor);
	}

	// Raylib's DrawTextPro takes a per-character pixel spacing. SFML's
	// letterSpacing is a multiplier on the space glyph's width. Phase 1
	// approximates: 0 spacing at factor 1.0, +1 px per unit factor over 1.0.
	// Good enough for the RPG-UI use case; Phase 2 may route per-glyph.
	float spacingFromFactor(float factor) noexcept {
		return std::max(0.f, (factor - 1.f) * 2.f);
	}

	// A frame is drawn without the GVL while Ruby threads keep running: one
	// may (re)build a text's layout (Text#x= measures it) while the frame
	// reads it.
	std::mutex g_layoutMutex;

}

void cgss::ScrollText::setString(const std::string& utf8) {
	// '\r' is skipped, as SFML's text does: CRLF texts (PSDK's error log)
	// would show raylib's '?' fallback glyph at every line end.
	m_utf8.clear();
	m_utf8.reserve(utf8.size());
	for (const char c : utf8) {
		if (c != '\r') m_utf8.push_back(c);
	}
	ensureGlyphs();
	invalidateLayout();
}

void cgss::ScrollText::setFont(const ::Font& font) {
	m_font = &font;
	ensureGlyphs();
	applyFilter();
	invalidateLayout();
}

// The font's atlas gets the characters of this text it lacks (glyphs on
// demand, see Internal/RaylibFontAtlas.h); a rebuilt atlas needs the filter.
void cgss::ScrollText::ensureGlyphs() {
	if (m_font == nullptr || m_utf8.empty()) return;
	const auto texture = m_font->texture.id;
	cgss::backend::raylib_detail::ensureFontGlyphs(const_cast<::Font&>(*m_font), m_utf8.data(), m_utf8.size());
	if (m_font->texture.id != texture) applyFilter();
}

void cgss::ScrollText::applyFilter() {
	if (m_font != nullptr && m_font->texture.id != 0) {
		::SetTextureFilter(m_font->texture,
		                   m_smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
	}
}

void cgss::ScrollText::setCharacterSize(unsigned int size) {
	m_characterSize = size;
	invalidateLayout();
}

void cgss::ScrollText::setFillColor(cgss::Color c) {
	m_fill = ::Color{ c.r, c.g, c.b, c.a };
}

void cgss::ScrollText::setOutlineColor(cgss::Color c) {
	m_outline = ::Color{ c.r, c.g, c.b, c.a };
}

void cgss::ScrollText::setOutlineThickness(float thickness) {
	m_outlineThickness = thickness;
	// As SFML's (LiteRGSS2): an outline replaces the shadow, none brings it
	// back; draw_shadow= after it still decides
	m_drawShadow = (thickness < 1.0f);
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
	invalidateLayout();
}

void cgss::ScrollText::setLetterSpacing(float factor) {
	m_letterSpacingFactor = factor;
	invalidateLayout();
}

void cgss::ScrollText::setLineSpacing(float factor) {
	m_lineSpacingFactor = factor;
	invalidateLayout();
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

void cgss::ScrollText::ensureLayout() const {
	const bool atlasChanged = m_font != nullptr
		&& (m_font->glyphs != m_layoutGlyphs || m_font->glyphCount != m_layoutGlyphCount
		    || m_font->texture.id != m_layoutTexture);
	if (!m_layoutDirty && !atlasChanged) return;
	m_layoutDirty = false;
	m_layout.clear();
	m_inkBounds = cgss::FloatRect{ 0.f, 0.f, 0.f, 0.f };
	m_layoutGlyphs     = m_font ? m_font->glyphs : nullptr;
	m_layoutGlyphCount = m_font ? m_font->glyphCount : 0;
	m_layoutTexture    = m_font ? m_font->texture.id : 0;
	if (!m_font || m_utf8.empty() || m_font->baseSize <= 0 || m_font->glyphCount <= 0) return;

	const float fs       = fontSizeF(m_characterSize);
	const float scale    = fs / static_cast<float>(m_font->baseSize);
	const float spacing  = spacingFromFactor(m_letterSpacingFactor);
	const float lineStep = lineStepOf(m_lineHeight, m_lineSpacingFactor);

	// Pen positions, stepped as raylib's DrawTextEx steps them (lines by
	// SetTextLineSpacing's integer step, a glyph without advance by its
	// width), so drawing the layout matches DrawTextPro pixel for pixel.
	float penX = 0.f, penY = 0.f;
	// Ink box, as SFML's ScrollText measures it: the glyphs' ink extents (not
	// their advances — the last glyph's right bearing doesn't count),
	// whitespace advancing the max. Text alignment relies on this width.
	float x = 0.f, y = 0.f;
	float minX = fs, minY = fs, maxX = 0.f, maxY = 0.f;

	m_layout.reserve(m_utf8.size());
	const char* text = m_utf8.c_str();
	const char* end  = text + m_utf8.size();
	while (text < end) {
		int bytes = 0;
		const int cp = ::GetCodepointNext(text, &bytes);
		if (bytes <= 0) break; // utf8BytesForCodepoints' count stops here too
		text += bytes;
		const int index = ::GetGlyphIndex(*m_font, cp); // '?' when the atlas lacks cp
		const ::GlyphInfo& glyph = m_font->glyphs[index];
		const ::Rectangle& rec   = m_font->recs[index];
		const float advance = static_cast<float>(glyph.advanceX) * scale + spacing;

		if (cp == '\n') {
			m_layout.push_back({ -1, penX, penY });
			penY += static_cast<float>(static_cast<int>(lineStep));
			penX = 0.f;
		} else {
			const bool blank = cp == ' ' || cp == '\t';
			m_layout.push_back({ blank ? -1 : index, penX, penY });
			penX += glyph.advanceX == 0 ? rec.width * scale + spacing : advance;
		}

		if (cp == ' ' || cp == '\t' || cp == '\n') {
			minX = std::min(minX, x);
			minY = std::min(minY, y);
			if (cp == ' ') x += advance;
			else if (cp == '\t') x += advance * 4.f;
			else { y += lineStep; x = 0.f; }
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
			continue;
		}
		const float left = x + static_cast<float>(glyph.offsetX) * scale;
		const float top  = y + static_cast<float>(glyph.offsetY) * scale;
		minX = std::min(minX, left);
		maxX = std::max(maxX, left + rec.width * scale);
		minY = std::min(minY, top);
		maxY = std::max(maxY, top + rec.height * scale);
		x += advance;
	}
	m_inkBounds = cgss::FloatRect{ minX, minY, maxX - minX, maxY - minY };
}

cgss::FloatRect cgss::ScrollText::getLocalBounds() const {
	if (!m_font || m_utf8.empty() || m_font->baseSize <= 0) return cgss::FloatRect{ 0.f, 0.f, 0.f, 0.f };
	cgss::FloatRect bounds;
	{
		std::lock_guard<std::mutex> lock(g_layoutMutex);
		ensureLayout();
		bounds = m_inkBounds;
	}
	// The outline expands the ink box.
	if (m_outlineThickness != 0.f) {
		const float outline = std::fabs(std::ceil(m_outlineThickness));
		bounds.left   -= outline;
		bounds.top    -= outline;
		bounds.width  += 2.f * outline;
		bounds.height += 2.f * outline;
	}
	return bounds;
}

// What DrawTextPro(font, text, offset, origin, rotation, ...) draws, from the
// layout: same matrix, and DrawTextCodepoint's quads (`padding` standing for
// the font's glyphPadding).
void cgss::ScrollText::drawGlyphs(const ::Font& font, int padding, ::Vector2 offset, ::Vector2 origin,
                                  float rotation, std::size_t count, ::Color tint) const {
	const float scale = fontSizeF(m_characterSize) / static_cast<float>(font.baseSize);
	const float pad   = static_cast<float>(padding);
	::rlPushMatrix();
	::rlTranslatef(offset.x, offset.y, 0.f);
	::rlRotatef(rotation, 0.f, 0.f, 1.f);
	::rlTranslatef(-origin.x, -origin.y, 0.f);
	for (std::size_t i = 0; i < count; ++i) {
		const LaidGlyph& g = m_layout[i];
		if (g.index < 0) continue;
		const ::GlyphInfo& glyph = font.glyphs[g.index];
		const ::Rectangle& rec   = font.recs[g.index];
		const ::Rectangle dst{
			g.x + (static_cast<float>(glyph.offsetX) - pad) * scale,
			g.y + (static_cast<float>(glyph.offsetY) - pad) * scale,
			(rec.width + 2.f * pad) * scale,
			(rec.height + 2.f * pad) * scale };
		const ::Rectangle src{ rec.x - pad, rec.y - pad, rec.width + 2.f * pad, rec.height + 2.f * pad };
		::DrawTexturePro(font.texture, src, dst, ::Vector2{ 0.f, 0.f }, 0.f, tint);
	}
	::rlPopMatrix();
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

	std::lock_guard<std::mutex> lock(g_layoutMutex);
	ensureLayout();
	const std::size_t count = std::min<std::size_t>(m_layout.size(), m_numCharsToDraw);
	if (count == 0) return;

	const float fs      = fontSizeF(m_characterSize);
	const float spacing = spacingFromFactor(m_letterSpacingFactor);
	const float lineStep = lineStepOf(m_lineHeight, m_lineSpacingFactor);
	// MeasureTextEx (underline widths below) reads this raylib global.
	::SetTextLineSpacing(static_cast<int>(lineStep));

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
		drawGlyphs(*m_font, 0, ::Vector2{ 1.f, 0.f }, origin, rot, count, m_outline);
		drawGlyphs(*m_font, 0, ::Vector2{ 1.f, 1.f }, origin, rot, count, m_outline);
		drawGlyphs(*m_font, 0, ::Vector2{ 0.f, 1.f }, origin, rot, count, m_outline);
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

			// The atlas leaves Font::glyphPadding at 0, so glyph quads are
			// tight to the glyph's bitmap bounds — leaving no room for
			// outline pixels outside the glyph (where SFML's baked outline
			// lives). Draw the quads padded by ceil(thickness), clamped to
			// the GenImageFontAtlas inter-glyph padding (4 px) so the
			// expanded source rect samples the pre-zeroed gutter, never
			// into neighbouring glyphs.
			const int padding = std::clamp(
				static_cast<int>(std::ceil(m_outlineThickness)), 1, 4);

			::BeginShaderMode(s.shader);
			drawGlyphs(*m_font, padding, ::Vector2{ 0.f, 0.f }, origin, rot, count, m_outline);
			::EndShaderMode();
		} else {
			drawGlyphs(*m_font, 0, ::Vector2{ 1.f, 1.f }, origin, rot, count, m_outline);
		}
	}

	// Fill pass. Synthetic bold re-stamps at +1px — raylib has no synthetic
	// bold, and proper bold requires a separately-loaded font variant; this
	// thickens the stroke as a best-effort fallback.
	drawGlyphs(*m_font, 0, ::Vector2{ 0.f, 0.f }, origin, rot, count, m_fill);
	if (drawBold) {
		drawGlyphs(*m_font, 0, ::Vector2{ 1.f, 0.f }, origin, rot, count, m_fill);
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
		const std::string truncated = m_utf8.substr(0, utf8BytesForCodepoints(m_utf8, static_cast<std::uint32_t>(count)));
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
			const float yLineTop = lineStep * static_cast<float>(lineIndex) - m_oy;
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
