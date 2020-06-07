#ifndef CGSS_VIEWPORT_H
#define CGSS_VIEWPORT_H
#include <optional>
#include "Graphics/ShaderRenderable.h"
#include "View.h"
#include "ViewAuthorizations.h"
#include "ViewBox.h"
#include "Common/Meta/metadata.h"
#include "Graphics/RenderStates.h"

namespace cgss {
	
	class Viewport;
	class ViewportItem : 
		public GraphicsStackItem,
		public View,
		public ViewBox,
		public ShaderRenderable {
		friend class Viewport;
	private:
		const sf::RenderStates& getRenderState() const;
		void setupFullWindowView(sf::RenderTarget& target) const;
		void onPostWindowSettingsChange(WindowSettingsChangeEvent& event) override;

		sf::Color m_color;
		sf::Sprite m_renderSprite;
		std::optional<sf::RenderStates> m_renderStates;
		RenderStates m_defaultRenderStates;

		mutable sf::RenderTexture m_render;

		bool m_smooth = false;
		float m_wantedOx = 0;
		float m_wantedOy = 0;
	public:
		void createRender();
		ViewportItem(Viewport* owner, MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings);
		void drawFast(sf::RenderTarget& target) const override { draw(target); }
		void draw(sf::RenderTarget& target) const override;
		void updateFromRectangle(const sf::IntRect& rectangle) override;
		sf::IntRect getViewportBox() const;
	private:
		sf::Shader& getRenderStateShader() const;
	};

	struct DisplayWindowSettings;
	class Viewport :
		public Drawable<Viewport, ViewportItem>,
		public GraphicItem {

	public:
		Viewport();
		Viewport(Viewport&&) noexcept = default;
		Viewport& operator=(Viewport&&) noexcept = default;
		Viewport(const Viewport&) = delete;
		Viewport& operator=(const Viewport&) = delete;
		virtual ~Viewport();

		void bindRenderStates(RenderStates* states);
		void drawFast(sf::RenderTarget& target) const override;
		void draw(sf::RenderTarget& target) const override;

		void moveOrigin(long ox, long oy);
		void move(float x, float y);
		void resize(long width, long height);
		void setRectangle(sf::IntRect rectangle);
		sf::IntRect getViewportBox() const;

		long getOx() const;
		long getOy() const;

		void setAngle(float angle);
		void setScale(float zoom);

		template <class View, class StackItem, class Drawable>
		StackItem& add(std::shared_ptr<StackItem> d) {
			return m_proxy->View::add<Viewport, StackItem, Drawable>(std::move(d));
		}
	private:
		void onDispose() override;
	};

	class Sprite;
	class Text;
	class Shape;
	class FramedView;
	class SpriteMap;

	template <>
	struct ViewAuthorizations<Viewport> : public AuthorizedDrawableSet<Sprite, SpriteMap, Text, Shape, FramedView> {};

	namespace meta {
		template<>
		struct Log<Viewport> {
			static constexpr auto classname = "Viewport";
		};
	}
}

#endif