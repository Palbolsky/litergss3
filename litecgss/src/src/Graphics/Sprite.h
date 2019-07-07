#ifndef CGSS_SPRITE_H
#define CGSS_SPRITE_H
#include "Graphics/Drawable.h"
#include "Graphics/Transformable.h"
#include "Common/Meta/metadata.h"

namespace cgss {
	class Texture;
	class Sprite :
		public Drawable<Sprite>,
		public Transformable {
	private:
		void horizontalFlip();

		sf::Sprite m_sprite;
		bool m_visible = true;
		bool m_mirror = false;

		sf::RenderStates m_renderStates = sf::RenderStates::Default;
		
	public:
		Sprite();
		Sprite(Sprite&&) = default;
		Sprite(const Sprite&) = delete;
		Sprite& operator=(const Sprite&) = delete;
		Sprite& operator=(Sprite&&) = default;
		virtual ~Sprite() = default;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		void setVisible(bool value) { m_visible = value;};
		bool isVisible() const { return m_visible; };

		sf::Color getColor() const;
		void setColor(sf::Color color);

		void setRenderStates(sf::RenderStates renderStates);

		void setMirror(bool mirror);
	};

	namespace meta {
		template<>
		struct Log<Sprite> {
			static constexpr auto classname = "Sprite";
		};
	}
}
#endif
