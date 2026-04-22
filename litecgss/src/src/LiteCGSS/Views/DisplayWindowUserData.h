#ifndef CGSS_DISPLAY_WINDOW_USER_DATA_H
#define CGSS_DISPLAY_WINDOW_USER_DATA_H
#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Events/MainEventDispatcher.h"
#include "DisplayWindowFwd.h"

namespace cgss {
	struct DisplayWindowSettings;

	template <class B>
	struct basic_DisplayWindowUserData {
		MainEventDispatcher& eventDispatcher;
		const DisplayWindowSettings& settings;

		/* Shader related stuff (used by Viewports) */
		std::unique_ptr<typename B::native_sprite>& userRenderSprite;
		std::unique_ptr<typename B::native_render_texture>& userRenderTexture;
	};

	using DisplayWindowUserData = basic_DisplayWindowUserData<cgss::backend::ActiveBackend>;
}

#endif
