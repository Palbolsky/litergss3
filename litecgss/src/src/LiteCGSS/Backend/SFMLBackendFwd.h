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

	// Plain POD return type for size queries from the backend. Used to avoid
	// leaking native vector types (sf::Vector2u) into wrapper signatures.
	struct Size2u {
		unsigned int x;
		unsigned int y;
	};

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

		// All inline thunks live on the nested Ops struct, defined in
		// SFMLBackend.h. Forward-declared here so wrappers can name
		// `B::Ops::method` in member declarations / dependent contexts.
		struct Ops;
	};

}

#endif
