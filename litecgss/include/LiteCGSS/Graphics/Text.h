#ifndef CGSS_TEXT_H
#define CGSS_TEXT_H
#include <memory>
#include <string>
#include <cstdint>
#include "LiteCGSS/Graphics/Font.h"
#include "LiteCGSS/Graphics/Drawable.h"
#include "LiteCGSS/Graphics/Transformable.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/Meta/metadata.h"

namespace cgss {
	class Text;
	class View;
	// ScrollText is SFML-coupled on purpose and lives in the PRIVATE tree
	// (src/LiteCGSS/Graphics/ScrollText.h). TextItem owns it via unique_ptr
	// so this public header only needs a forward declaration — consumers
	// never pull <SFML/...> through Text.h.
	class ScrollText;
	class TextItem : public GraphicsStackItem {
		friend class Text;
	public:
		TextItem(Text* owner);
		// Out-of-line destructor: unique_ptr<ScrollText> requires the complete
		// type at the point of destruction, which is only known inside the
		// backend-specific Text.cpp.
		~TextItem();
		void draw(cgss::RenderTarget& target) const override;
		void drawFast(cgss::RenderTarget& target) const override;
		void updateAlign();
		void setOpacity(std::uint8_t opacity) override;
	private:
		std::unique_ptr<ScrollText> m_text;
		uint8_t m_align = 0;
		long m_width = 1;
		long m_height = 1;
		float m_wantedX = 0.f;
		float m_wantedY = 0.f;
	};

	class Text :
		public Drawable<Text, TextItem>,
		private Transformable {

	public:
		Text() = default;
		Text(Text&&) = default;
		Text(const Text&) = delete;
		Text& operator=(const Text&) = delete;
		Text& operator=(Text&&) = default;
		virtual ~Text() = default;

		std::uint8_t getOpacity() const;
		void setOpacity(std::uint8_t opacity);
		float getRealWidth() const;
		float getRealHeight() const;
		cgss::Color getFillColor() const;
		void setFillColor(cgss::Color color);
		cgss::Color getOutlineColor() const;
		void setOutlineColor(cgss::Color color);
		float getOutlineThickness() const;
		void setOutlineThickness(float outlineThickness);
		std::uint32_t getStyle() const;
		void setStyle(std::uint32_t style);
		void setAlign(uint8_t align);
		long getCharacterSize() const;
		void setCharacterSize(long characterSize);
		void setString(const std::string& text);
		void setFont(const cgss::Font& font);

		void setDrawShadow(bool drawshadow);
		bool getDrawShadow() const;
		void setNumCharToDraw(std::uint32_t num);
		std::uint32_t getNumCharToDraw() const;
		std::uint32_t getTextWidth(const std::string& string) const;
		void setSmooth(bool smooth);
		bool getSmooth() const;

		float getX() const;
		float getY() const;

		using Transformable::getOx;
		using Transformable::getOy;
		using Transformable::getScaleX;
		using Transformable::getScaleY;
		using Transformable::moveOrigin;

		void scale(float xScale, float yScale);
		void move(float x, float y);
		void resize(long width, long height);
		long getWidth() const;
		long getHeight() const;
	private:
		void setLineHeight(float height);
	};

	namespace meta {
		template<>
		struct Log<Text> {
			static constexpr auto classname = "Text";
		};
	}
}
#endif
