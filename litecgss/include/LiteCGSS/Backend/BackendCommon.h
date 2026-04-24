#ifndef CGSS_BACKEND_COMMON_H
#define CGSS_BACKEND_COMMON_H

// Backend-agnostic POD types used by every backend's Fwd / Ops surface.
// Pulled into both `SFMLBackendFwd.h` and `RaylibBackendFwd.h` so that ops
// like `get_texture_size(...) -> Size2u` mean the same thing on any backend.

namespace cgss::backend {

	// Plain POD for size queries — avoids leaking native vector types
	// (sf::Vector2u under SFML, raylib has no equivalent struct).
	struct Size2u {
		unsigned int x = 0u;
		unsigned int y = 0u;
	};

	// Float vector (for view size / center / point returns).
	struct Size2f {
		float x = 0.f;
		float y = 0.f;
	};

	// Float-typed rectangle (for view viewport returns — SFML expresses it
	// as a normalized [0..1] rect). Field naming mirrors sf::FloatRect for
	// ease of mapping.
	struct Box2f {
		float left   = 0.f;
		float top    = 0.f;
		float width  = 0.f;
		float height = 0.f;
	};

	// Signed integer 2D point (for window position get/set).
	struct Point2i {
		int x = 0;
		int y = 0;
	};

	// Selectors for the four blend-factor slots and two blend-equation slots
	// in a draw state. Used by draw_state_get/set_blend_factor/equation ops
	// to avoid 12 separate methods.
	enum class BlendFactorSlot   { ColorSrc, ColorDst, AlphaSrc, AlphaDst };
	enum class BlendEquationSlot { Color, Alpha };

}

namespace cgss { struct DisplayWindowContextSettings; }

namespace cgss::backend {

	// Options POD for window creation — replaces the SFML-typed
	// (sf::VideoMode + sf::String + sf::Style + sf::ContextSettings) bundle.
	// Each backend's `window_create(window, options, title)` op interprets
	// these its own way: SFML maps them directly to .create() args; raylib
	// applies SetConfigFlags + InitWindow (or no-ops if its global window
	// is already initialized).
	//
	// `vsync` is only read pre-init by raylib (FLAG_VSYNC_HINT must be set
	// before InitWindow). SFML accepts vsync post-create, so it ignores this
	// field and relies on the subsequent `window_set_vsync` call.
	struct WindowCreateOptions {
		unsigned int width        = 0u;
		unsigned int height       = 0u;
		unsigned int bitsPerPixel = 32u;
		bool         fullscreen   = false;
		bool         vsync        = false;
		const cgss::DisplayWindowContextSettings* context = nullptr;
	};

}

#endif
