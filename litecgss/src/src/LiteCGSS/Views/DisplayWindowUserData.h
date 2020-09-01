#ifndef CGSS_DISPLAY_WINDOW_USER_DATA_H
#define CGSS_DISPLAY_WINDOW_USER_DATA_H
#include <memory>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

namespace cgss {
	struct DisplayWindowUserData {
		/* Shader related stuff (used by Viewports) */
		sf::Sprite* userRenderSprite = nullptr;
		sf::RenderTexture* userRenderTexture = nullptr;
	};
}

#endif