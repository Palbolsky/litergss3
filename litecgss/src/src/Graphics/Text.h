#ifndef CGSS_TEXT_H
#define CGSS_TEXT_H
#include "Graphics/Drawable.h"
#include "Common/Meta/metadata.h"

namespace cgss {
	class Text :
		public Drawable<Text> {
	private:
		sf::Text m_text;
	public:
		Text() = default;
		virtual ~Text() = default;
		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		sf::Text& getText() { return m_text;}
	};
	namespace meta {
		template<>
		struct Log<Text> {
			static constexpr auto classname = "Text";
		};
	}
}
#endif
