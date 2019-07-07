#ifndef LR_TEXT_H
#define LR_TEXT_H
#include "Graphics/Stack/Drawable.h"
#include "Common/Meta/metadata.h"

namespace lr {
	class Text :
		public Drawable<Text> {
	private:
		sf::Text m_text;
		bool m_visible = true;
	public:
		Text() = default;
		virtual ~Text() = default;
		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		sf::Text& getText() { return m_text;}
		void setVisible(bool value) { m_visible = value; }
		bool getVisible() { return m_visible; };
	};
	namespace meta {
		template<>
		struct Log<Text> {
			static constexpr auto classname = "Text";
		};
	}
}
#endif
