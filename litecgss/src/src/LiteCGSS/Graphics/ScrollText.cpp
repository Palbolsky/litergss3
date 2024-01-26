/*

LiteCGSS integration Note :
THIS SOURCE FILE IS AN ALTERED VERSION OF sf::Text FOUND IN Text.cpp OF SFML GRAPHICS MODULE
IF YOU WANT TO CHECK THE ORIGINAL FILE, LOOK AT SFML IMPLEMENTATION.

*/

/*
SFML (Simple and Fast Multimedia Library) - Copyright (c) Laurent Gomila

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
   that you wrote the original software. If you use this software in a product,
   an acknowledgment in the product documentation would be appreciated but is
   not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
*/

#include "ScrollText.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <cmath>

// Add an underline or strikethrough line to the vertex array
static void addLine(sf::VertexArray& vertices, float lineLength, float lineTop, const sf::Color& color, float offset, float thickness, float outlineThickness = 0) {
  const float top    = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
  const float bottom = top + std::floor(thickness + 0.5f);

  vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
  vertices.append(sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
  vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
  vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
  vertices.append(sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
  vertices.append(sf::Vertex(sf::Vector2f(lineLength + outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
}

// Add a glyph quad to the vertex array
static void addGlyphQuad(sf::VertexArray& vertices, sf::Vector2f position, const sf::Color& color, const sf::Glyph& glyph, float italicShear) {
  const float padding = 1.0;

  const float left   = glyph.bounds.left - padding;
  const float top    = glyph.bounds.top - padding;
  const float right  = glyph.bounds.left + glyph.bounds.width + padding;
  const float bottom = glyph.bounds.top + glyph.bounds.height + padding;

  const float u1 = static_cast<float>(glyph.textureRect.left) - padding;
  const float v1 = static_cast<float>(glyph.textureRect.top) - padding;
  const float u2 = static_cast<float>(glyph.textureRect.left + glyph.textureRect.width) + padding;
  const float v2 = static_cast<float>(glyph.textureRect.top + glyph.textureRect.height) + padding;

  vertices.append(sf::Vertex(sf::Vector2f(position.x + left - italicShear * top, position.y + top), color, sf::Vector2f(u1, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
  vertices.append(sf::Vertex(sf::Vector2f(position.x + right - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u2, v2)));
}

cgss::ScrollText::ScrollText() :
	m_font(NULL),
	m_characterSize(30),
	m_style(Regular),
	m_fillColor(255, 255, 255),
	m_outlineColor(0, 0, 0),
	m_vertices(sf::Triangles),
	m_outlineVertices(sf::Triangles),
	m_geometryNeedUpdate(false) {
}

cgss::ScrollText::ScrollText(const sf::String& string, const sf::Font& font, unsigned int characterSize) :
	m_string(string),
	m_font(&font),
	m_characterSize(characterSize),
	m_style(Regular),
	m_fillColor(255, 255, 255),
	m_outlineColor(0, 0, 0),
	m_outlineThickness(0),
	m_vertices(sf::Triangles),
	m_outlineVertices(sf::Triangles),
	m_geometryNeedUpdate(true) {
}

void cgss::ScrollText::setString(const sf::String& string) {
	if (m_string != string) {
		m_string = string;
		m_geometryNeedUpdate = true;
		if (m_font != nullptr && !m_smooth) {
			const_cast<sf::Texture*>(&m_font->getTexture(m_characterSize))->setSmooth(false);
		}
	}
}

void cgss::ScrollText::setSmooth(bool smooth) {
	m_smooth = smooth;
}

bool cgss::ScrollText::getSmooth() const {
	return m_smooth;
}

void cgss::ScrollText::setDrawShadow(bool drawshadow) {
	m_drawShadow = drawshadow;
	m_geometryNeedUpdate = true;
}

bool cgss::ScrollText::getDrawShadow() const {
	return m_drawShadow;
}
void cgss::ScrollText::setNumCharToDraw(sf::Uint32 num) {
	m_numberCharsToDraw = num;
	m_geometryNeedUpdate = true;
}

sf::Uint32 cgss::ScrollText::getNumCharToDraw() const {
	return m_numberCharsToDraw;
}

void cgss::ScrollText::setLineHeight(float height) {
	m_lineHeight = height;
}

void cgss::ScrollText::setFont(const sf::Font& font) {
	if (m_font != &font) {
		m_font = &font;
		m_geometryNeedUpdate = true;
		if (m_font != nullptr && !m_smooth) {
			const_cast<sf::Texture*>(&m_font->getTexture(m_characterSize))->setSmooth(false);
		}
	}
}

void cgss::ScrollText::setCharacterSize(unsigned int size) {
	if (m_characterSize != size) {
		m_characterSize = size;
		m_geometryNeedUpdate = true;
		if (m_font != nullptr && !m_smooth) {
			const_cast<sf::Texture*>(&m_font->getTexture(m_characterSize))->setSmooth(false);
		}
	}
}

void cgss::ScrollText::setLetterSpacing(float spacingFactor) {
	if (m_letterSpacingFactor != spacingFactor) {
		m_letterSpacingFactor = spacingFactor;
		m_geometryNeedUpdate = true;
	}
}

void cgss::ScrollText::setLineSpacing(float spacingFactor) {
	if (m_lineSpacingFactor != spacingFactor) {
		m_lineSpacingFactor = spacingFactor;
		m_geometryNeedUpdate = true;
	}
}

void cgss::ScrollText::setStyle(sf::Uint32 style) {
	if (m_style != style) {
		m_style = style;
		m_geometryNeedUpdate = true;
	}
}

void cgss::ScrollText::setFillColor(const sf::Color& color) {
	if (color != m_fillColor) {
		m_fillColor = color;

		// Change vertex colors directly, no need to update whole geometry
		// (if geometry is updated anyway, we can skip this step)
		if (!m_geometryNeedUpdate) {
			for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i) {
				m_vertices[i].color = m_fillColor;
			}
		}
	}
}

void cgss::ScrollText::setOutlineColor(const sf::Color& color) {
	if (color != m_outlineColor) {
		m_outlineColor = color;

		// Change vertex colors directly, no need to update whole geometry
		// (if geometry is updated anyway, we can skip this step)
		if (!m_geometryNeedUpdate) {
			for (std::size_t i = 0; i < m_outlineVertices.getVertexCount(); ++i) {
				m_outlineVertices[i].color = m_outlineColor;
			}
		}
	}
}

void cgss::ScrollText::setOutlineThickness(float thickness) {
	if (thickness != m_outlineThickness) {
		m_outlineThickness = thickness;
		m_drawShadow = (thickness < 1.0f);
		m_geometryNeedUpdate = true;
	}
}

const sf::String& cgss::ScrollText::getString() const {
	return m_string;
}

const sf::Font* cgss::ScrollText::getFont() const {
	return m_font;
}

unsigned int cgss::ScrollText::getCharacterSize() const {
	return m_characterSize;
}

float cgss::ScrollText::getLetterSpacing() const {
	return m_letterSpacingFactor;
}

float cgss::ScrollText::getLineSpacing() const {
	return m_lineSpacingFactor;
}

sf::Uint32 cgss::ScrollText::getStyle() const {
	return m_style;
}

const sf::Color& cgss::ScrollText::getFillColor() const {
	return m_fillColor;
}

const sf::Color& cgss::ScrollText::getOutlineColor() const {
	return m_outlineColor;
}

float cgss::ScrollText::getOutlineThickness() const {
	return m_outlineThickness;
}

sf::Vector2f cgss::ScrollText::findCharacterPos(std::size_t index) const {
	// Make sure that we have a valid font
	if (!m_font) {
		return sf::Vector2f();
	}

	// Adjust the index if it's out of range
	if (index > m_string.getSize()) index = m_string.getSize();

	// Precompute the variables needed by the algorithm
	const bool  isBold = m_style & Bold;
	float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
	const float letterSpacing = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
	whitespaceWidth += letterSpacing;
	const float lineSpacing = m_lineHeight * m_lineSpacingFactor;

	sf::Vector2f position;
	std::uint32_t prevChar = 0;
	for (std::size_t i = 0; i < index; ++i) {
		const std::uint32_t curChar = m_string[i];

		// Apply the kerning offset
		position.x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);
		prevChar = curChar;

		// Handle special characters
		switch (curChar) {
		case U' ':  position.x += whitespaceWidth;			 continue;
		case U'\t': position.x += whitespaceWidth * 4;		 continue;
		case U'\n': position.y += lineSpacing; position.x = 0; continue;
		}

		// For regular characters, add the advance offset of the glyph
		position.x += m_font->getGlyph(curChar, m_characterSize, isBold).advance + letterSpacing;
	}

	// Transform the position to global coordinates
	position = getTransform().transformPoint(position);

	return position;
}

sf::FloatRect cgss::ScrollText::getLocalBounds() const {
	ensureGeometryUpdate();

	return m_bounds;
}

sf::FloatRect cgss::ScrollText::getGlobalBounds() const {
	return getTransform().transformRect(getLocalBounds());
}

void cgss::ScrollText::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	if (m_font) {
		ensureGeometryUpdate();

    sf::RenderStates statesCopy(states);
	
		statesCopy.transform *= getTransform();
		statesCopy.texture = &m_font->getTexture(m_characterSize);

		// Only draw the outline if there is something to draw
		if (m_outlineThickness != 0 || m_drawShadow) {
			target.draw(m_outlineVertices, statesCopy);
		}
		target.draw(m_vertices, statesCopy);
	}
}

sf::Uint32 cgss::ScrollText::getTextWidth(const sf::String& string) const {
	// No font or cgss::ScrollText: nothing to draw
	if (!m_font || string.isEmpty()) {
		return 0;
	}

	// Compute values related to the text style
	const bool  isBold = m_style & Bold;
	const float italicShear = (m_style & Italic) ? 0.209f : 0.f; // 12 degrees in radians

															// Compute the location of the strike through dynamically
															// We use the center point of the lowercase 'x' glyph as the reference
															// We reuse the underline thickness as the thickness of the strike through as well

	// Precompute the variables needed by the algorithm
	float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
	const float letterSpacing = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
	whitespaceWidth += letterSpacing;
	const float lineSpacing = m_lineHeight * m_lineSpacingFactor;
	float x = 0.f;
	float y = static_cast<float>(m_characterSize);

	// Create one quad for each character
	float minX = static_cast<float>(m_characterSize);
	float minY = static_cast<float>(m_characterSize);
	float maxX = 0.f;
	float maxY = 0.f;
	sf::Uint32 prevChar = 0;
	for (std::size_t i = 0; i < string.getSize(); ++i) {
		sf::Uint32 curChar = string[i];

		// Apply the kerning offset
		x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);
		prevChar = curChar;

		// Handle special characters
		if ((curChar == U' ') || (curChar == U'\n') || (curChar == U'\t')) {
			// Update the current bounds (min coordinates)
			minX = std::min(minX, x);
			minY = std::min(minY, y);

			switch (curChar) {
			case U' ':  x += whitespaceWidth;	 break;
			case U'\t': x += whitespaceWidth * 4; break;
			case U'\n': y += lineSpacing; x = 0;  break;
			}

			// Update the current bounds (max coordinates)
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);

			// Next glyph, no need to create a quad for whitespace
			continue;
		}

		if (m_outlineThickness != 0 && !m_drawShadow) {
			const sf::Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold, m_outlineThickness);

			float left = glyph.bounds.left;
			float top = glyph.bounds.top;
			float right = glyph.bounds.left + glyph.bounds.width;
			float bottom = glyph.bounds.top + glyph.bounds.height;

			// Update the current bounds with the outlined glyph bounds
			minX = std::min(minX, x + left - italicShear * bottom - m_outlineThickness);
			maxX = std::max(maxX, x + right - italicShear * top - m_outlineThickness);
			minY = std::min(minY, y + top - m_outlineThickness);
			maxY = std::max(maxY, y + bottom - m_outlineThickness);
		}

		// Extract the current glyph's description
		const sf::Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);

		// Update the current bounds with the non outlined glyph bounds
		if (m_drawShadow || m_outlineThickness == 0) {
			float left = glyph.bounds.left;
			float top = glyph.bounds.top;
			float right = glyph.bounds.left + glyph.bounds.width;
			float bottom = glyph.bounds.top + glyph.bounds.height;

			minX = std::min(minX, x + left - italicShear * bottom);
			maxX = std::max(maxX, x + right - italicShear * top);
			minY = std::min(minY, y + top);
			maxY = std::max(maxY, y + bottom);
		}

		// Advance to the next character
		x += glyph.advance + letterSpacing;
	}

	// Add 1 if drawing shadow
	if (m_drawShadow) {
		maxX += 1.f;
	}

	return static_cast<sf::Uint32>(maxX);
}

void cgss::ScrollText::ensureGeometryUpdate() const {
	if (!m_font) return;

	// Do nothing, if geometry has not changed and the font texture has not changed
	if (!m_geometryNeedUpdate && &m_font->getTexture(m_characterSize) == m_fontTextureRef) return;

	// Save the current fonts texture id
	m_fontTextureRef = &m_font->getTexture(m_characterSize);

	// Mark geometry as updated
	m_geometryNeedUpdate = false;

	// Clear the previous geometry
	m_vertices.clear();
	m_outlineVertices.clear();
	m_bounds = sf::FloatRect();

	// No text: nothing to draw
	if (m_string.isEmpty()) return;

	// Compute values related to the text style
	const bool  isBold = m_style & Bold;
	const bool  isUnderlined = m_style & Underlined;
	const bool  isStrikeThrough = m_style & StrikeThrough;
	const float italicShear = (m_style & Italic) ? 0.209f : 0.f; // 12 degrees in radians
	const float underlineOffset = m_font->getUnderlinePosition(m_characterSize);
	const float underlineThickness = m_font->getUnderlineThickness(m_characterSize);

	// Compute the location of the strike through dynamically
	// We use the center point of the lowercase 'x' glyph as the reference
	// We reuse the underline thickness as the thickness of the strike through as well
	sf::FloatRect xBounds = m_font->getGlyph(U'x', m_characterSize, isBold).bounds;
	const float strikeThroughOffset = xBounds.top + xBounds.height / 2.f;

	// Precompute the variables needed by the algorithm
	float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
	const float letterSpacing = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
	whitespaceWidth += letterSpacing;
	const float lineSpacing = m_lineHeight * m_lineSpacingFactor;
	float x = 0.f;
	float y = static_cast<float>(m_characterSize);

	// Create one quad for each character
	float minX = static_cast<float>(m_characterSize);
	float minY = static_cast<float>(m_characterSize);
	float maxX = 0.f;
	float maxY = 0.f;
	std::uint32_t prevChar = 0;

	for (std::size_t i = 0; i < m_string.getSize() && i < m_numberCharsToDraw; ++i) {
		std::uint32_t curChar = m_string[i];

		// Skip the \r char to avoid weird graphical issues
    if (curChar == U'\r') continue;

		// Apply the kerning offset
		x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);

		// If we're using the underlined style and there's a new line, draw a line
		if (isUnderlined && (curChar == U'\n' && prevChar != U'\n')) {
			addLine(m_vertices, x, y, m_fillColor, underlineOffset, underlineThickness);

			if (m_outlineThickness != 0) {
				addLine(m_outlineVertices, x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
			}
		}

		// If we're using the strike through style and there's a new line, draw a line across all characters
		if (isStrikeThrough && (curChar == U'\n' && prevChar != U'\n'))  {
			addLine(m_vertices, x, y, m_fillColor, strikeThroughOffset, underlineThickness);

			if (m_outlineThickness != 0) {
				addLine(m_outlineVertices, x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
			}
		}

		prevChar = curChar;

		// Handle special characters
		if ((curChar == U' ') || (curChar == U'\n') || (curChar == U'\t')) {
			// Update the current bounds (min coordinates)
			minX = std::min(minX, x);
			minY = std::min(minY, y);

			switch (curChar) {
			case U' ':  x += whitespaceWidth;	 break;
			case U'\t': x += whitespaceWidth * 4; break;
			case U'\n': y += lineSpacing; x = 0;  break;
			}

			// Update the current bounds (max coordinates)
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);

			// Next glyph, no need to create a quad for whitespace
			continue;
		}

		// Apply the Shadow
		if (m_drawShadow) {
			// Extract the current glyph's description
			const sf::Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);
			addGlyphQuad(m_outlineVertices, sf::Vector2f(x + 1, y), m_outlineColor, glyph, italicShear);
			addGlyphQuad(m_outlineVertices, sf::Vector2f(x + 1, y + 1), m_outlineColor, glyph, italicShear);
			addGlyphQuad(m_outlineVertices, sf::Vector2f(x, y + 1), m_outlineColor, glyph, italicShear);
		} else if (m_outlineThickness != 0) {
			// Apply the outline
			const sf::Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold, m_outlineThickness);

			// Add the outline glyph to the vertices
			addGlyphQuad(m_outlineVertices, sf::Vector2f(x, y), m_outlineColor, glyph, italicShear);
		}

		// Extract the current glyph's description
		const sf::Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);

		// Add the glyph to the vertices
		addGlyphQuad(m_vertices, sf::Vector2f(x, y), m_fillColor, glyph, italicShear);

		// Update the current bounds with the non outlined glyph bounds
		float left = glyph.bounds.left;
		float top = glyph.bounds.top;
		float right = glyph.bounds.left + glyph.bounds.width;
		float bottom = glyph.bounds.top + glyph.bounds.height;

		minX = std::min(minX, x + left - italicShear * bottom);
		maxX = std::max(maxX, x + right - italicShear * top);
		minY = std::min(minY, y + top);
		maxY = std::max(maxY, y + bottom);

		// Advance to the next character
		x += glyph.advance + letterSpacing;
	}

	// If we're using outline, update the current bounds
	if (m_outlineThickness != 0)
	{
			const float outline = std::abs(std::ceil(m_outlineThickness));
			minX -= outline;
			maxX += outline;
			minY -= outline;
			maxY += outline;
	}

	// If we're using the underlined style, add the last line
	if (isUnderlined && (x > 0)) {
		addLine(m_vertices, x, y, m_fillColor, underlineOffset, underlineThickness);

		if (m_outlineThickness != 0) {
			addLine(m_outlineVertices, x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
		}
	}

	// If we're using the strike through style, add the last line across all characters
	if (isStrikeThrough && (x > 0)) {
		addLine(m_vertices, x, y, m_fillColor, strikeThroughOffset, underlineThickness);

		if (m_outlineThickness != 0) {
			addLine(m_outlineVertices, x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
		}
	}

	// Update the bounding rectangle
	m_bounds.left = minX;
	m_bounds.top = minY;
	m_bounds.width = maxX - minX;
	m_bounds.height = maxY - minY;
}
