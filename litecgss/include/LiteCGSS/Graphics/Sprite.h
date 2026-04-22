#ifndef CGSS_SPRITE_H
#define CGSS_SPRITE_H

#include <memory>
#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "LiteCGSS/Graphics/Drawable.h"
#include "LiteCGSS/Graphics/ShaderRenderable.h"
#include "LiteCGSS/Graphics/SpriteFwd.h"
#include "LiteCGSS/Graphics/TextureFwd.h"
#include "LiteCGSS/Graphics/Transformable.h"

namespace cgss {
	class View;
	class RenderStates;

	template <class B>
	class basic_SpriteItem :
		public GraphicsStackItem,
		public ShaderRenderable {
		friend class basic_Sprite<B>;
	private:
		std::unique_ptr<typename B::native_sprite> m_sprite;
		std::shared_ptr<typename B::native_texture> m_linkedTexture = nullptr;
		Transformable* m_transformable = nullptr;
		bool m_mirror = false;
		float m_wantedX = 0;
		float m_wantedY = 0;
	public:
		basic_SpriteItem(basic_Sprite<B>* owner);
		virtual ~basic_SpriteItem();
		void draw(cgss::RenderTarget& target) const override;
		void drawFast(cgss::RenderTarget& target) const override;
		void updateFromValue(const cgss::IntRect* rectangle) override;
		void updateFromValue(const RenderStatesData* value) override;
		void setOpacity(std::uint8_t opacity) override;
	};

	template <class B>
	class basic_Sprite :
		public Drawable<basic_Sprite<B>, basic_SpriteItem<B>>,
		public Transformable {
	private:
		void horizontalFlip();

	public:
		basic_Sprite() = default;
		basic_Sprite(basic_Sprite&&) = default;
		basic_Sprite(const basic_Sprite&) = delete;
		basic_Sprite& operator=(const basic_Sprite&) = delete;
		basic_Sprite& operator=(basic_Sprite&&) = default;
		virtual ~basic_Sprite() = default;

		float getX() const;
		float getY() const;
		void move(float x, float y);

		cgss::Color getColor() const;
		void setColor(cgss::Color color);

		void bindRenderStates(RenderStates* renderStates);
		void setTexture(basic_Texture<B>& texture, bool resetRect = false);
		void setTextureRect(cgss::IntRect rectangle);
		cgss::IntRect getTextureRect() const;
		void setMirror(bool mirror);
	};

	namespace meta {
		template <class B>
		struct Log<basic_Sprite<B>> {
			static constexpr auto classname = "Sprite";
		};
	}
}
#endif
