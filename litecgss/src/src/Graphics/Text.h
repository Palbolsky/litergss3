#ifndef CGSS_TEXT_H
#define CGSS_TEXT_H
#include "Graphics/Drawable.h"
#include "Common/Meta/metadata.h"

namespace cgss {
	class Text;
	class View;
	class TextItem : public GraphicsStackItem {
		friend class Text;
	public:
		TextItem(Text* owner) {}
		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;
	private:
		sf::Text m_text;
	};

	class Text :
		public Drawable<Text, TextItem> {

	public:
		virtual ~Text() = default;

		sf::Text& getText() { return m_proxy->m_text; }
	protected:
		Text() {};
	};
	namespace meta {
		template<>
		struct Log<Text> {
			static constexpr auto classname = "Text";
		};
	}
}
#endif
