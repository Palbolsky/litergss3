#ifndef CGSS_SPRITE_H
#define CGSS_SPRITE_H
#include "Graphics/Drawable.h"
#include "Common/Meta/metadata.h"

namespace cgss {
	class Sprite :
		public Drawable<Sprite> {
	private:
		sf::Sprite m_sprite;
		bool m_drawable = false;
		bool m_visible = true;
	public:
		Sprite() = default;
		Sprite(Sprite&&) = default;
		Sprite(const Sprite&) = delete;
		Sprite& operator=(const Sprite&) = delete;
		Sprite& operator=(Sprite&&) = default;
		virtual ~Sprite() = default;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		sf::Sprite& getSprite() { return m_sprite;};
		void setDrawable(bool value) { m_drawable = value;};
		bool getDrawable() { return m_drawable; };
		void setVisible(bool value) { m_visible = value;};
		bool getVisible() { return m_visible; };
	};

	namespace meta {
		template<>
		struct Log<Sprite> {
			static constexpr auto classname = "Sprite";
		};
	}
}
#endif
