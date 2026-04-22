#ifndef CGSS_BACKEND_SFML_BACKEND_FWD_H
#define CGSS_BACKEND_SFML_BACKEND_FWD_H

// Slim descriptor for the SFML backend: native typedefs + capability tags +
// fwd-decl of the nested Ops struct that holds all static inline thunks. NO
// SFML headers, NO function bodies.
//
// Public LiteCGSS templates include this so consumers can name
// `basic_X<SFMLBackend>` (and the compiler can lay out wrapper members of
// type `shared_ptr<sf::Texture>` etc.) without their TU pulling in
// <SFML/...>. Function bodies live in SFMLBackend.h, which defines
// `SFMLBackend::Ops` with all the inline static methods. Any TU that calls
// `Backend::Ops::*` must include the full SFMLBackend.h.

#include "LiteCGSS/Backend/BackendCommon.h"

namespace sf {
	class RenderTarget;
	class Texture;
	class Image;
	class Sprite;
	class View;
	class RenderStates;
	class Font;
	class Event;
	class RenderWindow;
	class RenderTexture;
	class Shader;
	class VertexArray;
}

namespace cgss::backend {

	struct SFMLBackend {
		// ---- Native typedefs ----
		using native_render_target  = sf::RenderTarget;
		using native_texture        = sf::Texture;
		using native_image          = sf::Image;
		using native_sprite         = sf::Sprite;
		using native_view           = sf::View;
		using native_render_states  = sf::RenderStates;
		using native_font           = sf::Font;
		using native_event          = sf::Event;
		using native_window         = sf::RenderWindow;
		using native_render_texture = sf::RenderTexture;
		using native_shader         = sf::Shader;
		using native_vertex_array   = sf::VertexArray;

		// ---- Capability tags ----
		static constexpr bool has_window      = true;
		static constexpr bool has_event_pump  = true;
		static constexpr bool owns_gpu_memory = true;
		// DisplayWindow selects `DisplayWindowSnapshot` when true, the no-op
		// `StubDisplayWindowSnapshot<...>` otherwise. Tag-based dispatch keeps
		// public headers from having to name individual backends.
		static constexpr bool has_snapshot    = true;
		// True when `native_window` is a real instance that owns per-window
		// state (close/clear/display/poll route through the instance). False
		// for backends whose window is a global singleton (raylib:
		// InitWindow/CloseWindow are free functions, `native_window` is a
		// placeholder). Callers that reach for instance-only ops
		// (window_set_active, window_poll_event, window_wait_event,
		// window_get_default_view, window_set_view, window_get_settings)
		// should gate on this tag.
		static constexpr bool window_is_stateful = true;

		// All inline thunks live on the nested Ops struct, defined in
		// SFMLBackend.h. Forward-declared here so wrappers can name
		// `B::Ops::method` in member declarations / dependent contexts.
		struct Ops;
	};

}

#endif
