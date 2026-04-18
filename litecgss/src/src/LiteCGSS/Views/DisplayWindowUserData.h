#ifndef CGSS_DISPLAY_WINDOW_USER_DATA_H
#define CGSS_DISPLAY_WINDOW_USER_DATA_H
#include <memory>

#include "LiteCGSS/Events/MainEventDispatcher.h"

namespace sf {
	class Sprite;
	class RenderTexture;
}

namespace cgss {
	class DisplayWindow;
	struct DisplayWindowSettings;

	struct DisplayWindowUserData {
		MainEventDispatcher& eventDispatcher;
		const DisplayWindowSettings& settings;

		/* Shader related stuff (used by Viewports) */
		std::unique_ptr<sf::Sprite>& userRenderSprite;
		std::unique_ptr<sf::RenderTexture>& userRenderTexture;
	};
}

#endif
