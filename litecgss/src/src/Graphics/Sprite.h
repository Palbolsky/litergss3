#ifndef CGSS_SPRITE_H
#define CGSS_SPRITE_H
#include "Graphics/Drawable.h"
#include "Graphics/Transformable.h"
#include "Common/Meta/metadata.h"

namespace cgss {
	class Texture;
	class Sprite;
	class View;

	class SpriteItem : public GraphicsStackItem {
		friend class Sprite;
	private:
		sf::Sprite m_sprite;
		bool m_mirror = false;

		sf::RenderStates m_renderStates = sf::RenderStates::Default;
	public:
		SpriteItem(Sprite* owner);
		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;
	};
	

	class Sprite :
		public Drawable<Sprite, SpriteItem>,
		public Transformable {
	private:
		void horizontalFlip();

	public:
		Sprite() = default;
		Sprite(Sprite&&) = default;
		Sprite(const Sprite&) = delete;
		Sprite& operator=(const Sprite&) = delete;
		Sprite& operator=(Sprite&&) = default;
		virtual ~Sprite() = default;

		sf::Color getColor() const;
		void setColor(sf::Color color);

		void setRenderStates(sf::RenderStates renderStates);
		void setTexture(Texture& texture);
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
