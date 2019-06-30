#ifndef CGSS_VIEWPORT_H
#define CGSS_VIEWPORT_H

#include "View.h"
#include "ViewAuthorizations.h"
#include "ViewBox.h"

namespace cgss {
	
	struct DisplayWindowSettings;
	class Viewport : 
		public View,
		public ViewBox,
		public Drawable<Viewport>,
		private SubObserver<WindowSettingsChangeEvent> {

	public:
		Viewport(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings);
		void drawFast(sf::RenderTarget& target) const override { draw(target); }
		void draw(sf::RenderTarget& target) const override;

		void setupView(sf::RenderTarget& target) const;
		void setRenderStates(sf::RenderStates* states);
		sf::RenderStates* getRenderState() const;
		sf::Shader* getRenderStateShader() const;
		sf::Color* checkUpColor() const;
		void createRender();

	private:
		bool onWindowSettingsChange(WindowSettingsChangeEvent& event);

		std::unique_ptr<sf::Color> m_colorCopy;
		std::unique_ptr<sf::Color> m_color;
		bool m_visible = true;
		std::unique_ptr<sf::RenderTexture> m_render;
		std::unique_ptr<sf::Sprite> m_renderSprite;
		sf::RenderStates* m_renderStates = nullptr;	
		std::unique_ptr<sf::RenderStates> m_defaultRenderStates;
		mutable sf::Shader* m_renderStatesShader = nullptr;
		mutable std::unique_ptr<sf::Shader> m_defaultRenderStatesShader;

		bool m_smooth = false;
	};

	class Sprite;
	class Text;
	class Shape;
	class FramedView;

	template <>
	struct ViewAuthorizations<Viewport> : public AuthorizedDrawableSet<Sprite, Text, Shape, FramedView> {};
}

#endif