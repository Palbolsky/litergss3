/*

LiteCGSS integration Note :
THIS SOURCE FILE IS AN ALTERED VERSION OF sf::Text FOUND IN Text.hpp OF SFML GRAPHICS MODULE
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

#ifndef CGSS_SCROLL_TEXT_H
#define CGSS_SCROLL_TEXT_H

#include <SFML/Graphics/Export.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/String.hpp>
#include <string>
#include <limits>
#include <vector>
#include <cstdint>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/FloatRect.h"

namespace cgss {

	// sf::Drawable stays a public base because sf::RenderTarget::draw(const
	// sf::Drawable&) is the SFML render path used by Ops::draw_text. The
	// sf::Transformable inheritance is private to keep its sf::Vector2f /
	// sf::Transform API out of ScrollText's public surface; the few methods
	// that the unified TextItem actually calls are re-exposed below.
	class ScrollText :
		public sf::Drawable,
		private sf::Transformable {
	public:
		using sf::Transformable::setPosition;
		using sf::Transformable::setOrigin;
		using sf::Transformable::setScale;
		using sf::Transformable::move;

		enum Style {
			Regular = 0,
			Bold = 1 << 0,
			Italic = 1 << 1,
			Underlined = 1 << 2,
			StrikeThrough = 1 << 3
		};

		ScrollText();
		ScrollText(const std::string& utf8, const sf::Font& font, unsigned int characterSize = 30);

		void setString(const std::string& utf8);
		void setFont(const sf::Font& font);
		void setCharacterSize(unsigned int size);
		void setLineSpacing(float spacingFactor);
		void setLetterSpacing(float spacingFactor);
		void setStyle(sf::Uint32 style);
		void setFillColor(cgss::Color color);
		void setOutlineColor(cgss::Color color);
		void setOutlineThickness(float thickness);
		const sf::Font* getFont() const;
		unsigned int getCharacterSize() const;
		float getLetterSpacing() const;
		float getLineSpacing() const;
		sf::Uint32 getStyle() const;
		cgss::Color getFillColor() const;
		cgss::Color getOutlineColor() const;
		float getOutlineThickness() const;
		cgss::FloatRect getLocalBounds() const;
		cgss::FloatRect getGlobalBounds() const;
		void setDrawShadow(bool drawshadow);
		bool getDrawShadow() const;
		void setNumCharToDraw(sf::Uint32 num);
		sf::Uint32 getNumCharToDraw() const;
		void setLineHeight(float height);
		std::uint32_t getTextWidth(const std::string& utf8) const;
		void setSmooth(bool smooth);
		bool getSmooth() const;

	private:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
		void ensureGeometryUpdate() const;

		sf::String m_string;
		const sf::Font* m_font = NULL;
		unsigned int m_characterSize;
		float m_letterSpacingFactor = 1.f;
		float m_lineSpacingFactor = 1.f;
		sf::Uint32 m_style{Regular};
		sf::Color m_fillColor{sf::Color::White};
		sf::Color m_outlineColor{sf::Color::Black};
		float m_outlineThickness = 0.f;
		mutable sf::VertexArray m_vertices{sf::PrimitiveType::Triangles};
		mutable sf::VertexArray m_outlineVertices{sf::PrimitiveType::Triangles};
		mutable sf::FloatRect m_bounds;
		mutable bool m_geometryNeedUpdate;
		mutable const sf::Texture* m_fontTextureRef = nullptr;
		bool m_drawShadow = true;
		sf::Uint32 m_numberCharsToDraw = std::numeric_limits<std::uint32_t>::max();
		float m_lineHeight = 16.0f;
		bool m_smooth = false;
	};
}


#endif
