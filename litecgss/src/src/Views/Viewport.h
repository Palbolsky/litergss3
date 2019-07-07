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
		public Drawable<Viewport> {

	public:
		Viewport(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings);
		void drawFast(sf::RenderTarget& target) const override { draw(target); }
		void draw(sf::RenderTarget& target) const override;

		void setupView(sf::RenderTarget& target) const;
		void setRenderStates(sf::RenderStates states);
		sf::Color* checkUpColor() const;
		void createRender();

	private:
		void onPostWindowSettingsChange(WindowSettingsChangeEvent& event);

		const sf::RenderStates& getRenderState() const;
		sf::Shader& getRenderStateShader() const;

		std::unique_ptr<sf::Color> m_colorCopy;
		std::unique_ptr<sf::Color> m_color;
		std::unique_ptr<sf::RenderTexture> m_render;
		std::unique_ptr<sf::Sprite> m_renderSprite;
		std::optional<sf::RenderStates> m_renderStates;
		sf::RenderStates m_defaultRenderStates = sf::RenderStates::Default;
		mutable sf::Shader* m_renderStatesShader = nullptr;
		mutable sf::Shader m_defaultRenderStatesShader;

		bool m_smooth = false;

		float rZ = 0.F;
		float rAngle = 0.F;
		float rZoom = 0.F;
	};

	class Sprite;
	class Text;
	class Shape;
	class FramedView;

	template <>
	struct ViewAuthorizations<Viewport> : public AuthorizedDrawableSet<Sprite, Text, Shape, FramedView> {};
}

#endif