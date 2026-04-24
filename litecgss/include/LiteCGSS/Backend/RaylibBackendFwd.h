#ifndef CGSS_BACKEND_RAYLIB_BACKEND_FWD_H
#define CGSS_BACKEND_RAYLIB_BACKEND_FWD_H

// Slim descriptor for the Raylib backend: native typedefs + capability tags +
// fwd-decl of the nested Ops struct. raylib's types are PODs that nest inside
// each other (Font has Texture inside, Camera2D has Vector2 inside) — clean
// forward-declaration would require duplicating the C struct layouts, so this
// header includes raylib.h directly. Consumer TUs that include cgss public
// headers under CGSS_BACKEND=raylib will absorb the raylib.h symbol set.
//
// Some cgss concepts have no native raylib equivalent (Window, Sprite, Event,
// RenderStates — raylib uses global state or per-frame polling). The
// placeholder PODs below let `basic_X<RaylibBackend>` declare members
// uniformly; the actual abstraction lives in Ops.

#include "LiteCGSS/Backend/BackendCommon.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Events/EventPayloads.h"
#include "LiteCGSS/Events/EventType.h"

#include <optional>

#include <raylib.h>

namespace cgss::backend {

	namespace raylib {
		// Raylib drives the actual window via global state (InitWindow /
		// CloseWindow / glfw singleton). This struct carries the tiny bit of
		// per-window state we need to patch around raylib/glfw quirks.
		//
		// `last_requested_position` — on X11, `glfwSetWindowPos` sends an
		// async ConfigureRequest and `GetWindowPosition` (via
		// XTranslateCoordinates) can report the pre-move value until the WM
		// processes it. SFML hides this by XSync'ing after XMoveWindow; raylib
		// doesn't. We cache the requested coords on set and return them on
		// get so the Ops contract ("get after set returns what was set")
		// matches SFMLBackend's observable behavior. Reset to nullopt before
		// any move so the first read after window creation still reflects the
		// real OS-assigned position.
		struct Window {
			std::optional<cgss::backend::Point2i> last_requested_position;
		};
		// Stateful sprite mirror of sf::Sprite. raylib's draw model is
		// immediate-mode (DrawTexturePro takes all draw args as parameters),
		// so there is no native "Sprite" to carry the configured state
		// between setter calls and the eventual draw. This struct fills that
		// gap — the `set_sprite_*` / `sprite_set_*` Ops write into these
		// fields; `draw_sprite[_with_transform]` reads them back and issues
		// the DrawTexturePro call.
		//
		// `texture` is a non-owning pointer — the cgss::basic_Sprite proxy
		// holds a shared_ptr<native_texture> in m_linkedTexture to keep the
		// ::Texture alive for the sprite's lifetime.
		struct Sprite {
			const ::Texture* texture = nullptr;
			cgss::Color      color { 255u, 255u, 255u, 255u };
			// {0,0,0,0} means "not explicitly set" — draw path will fall
			// back to the full texture extents at render time.
			cgss::IntRect    textureRect {};
			float            x = 0.f, y = 0.f;
			float            originX = 0.f, originY = 0.f;
			float            scaleX = 1.f, scaleY = 1.f;
		};
		struct RenderStates {};

		// sf::View-compatible view descriptor. raylib has Camera2D (offset /
		// target / zoom / rotation) but no built-in concept of a normalized
		// render-target viewport rect. cgss::ViewBox manipulates the view via
		// sf::View-style set_center / set_size / set_viewport, so the backend
		// owns the full 4-field model here and derives the effective
		// screen-pixel transform at draw time (see draw_sprite_impl in
		// RaylibBackend.h and raylib_detail::currentView()).
		struct View {
			float cx = 0.f, cy = 0.f;     // sf::View::setCenter — world coords
			float sizeX = 0.f, sizeY = 0.f; // sf::View::setSize — world-space window
			// sf::View::setViewport — normalized [0..1] rect on the render target.
			float vpX = 0.f, vpY = 0.f;
			float vpW = 1.f, vpH = 1.f;
			float rotation = 0.f;          // degrees, sf::View::setRotation
		};
		// Placeholder 2D transform — raylib draws with position/rotation/scale
		// as separate args, so there is no native "Transform" type. This POD
		// lets `basic_SpriteItem<RaylibBackend>` declare a
		// `unique_ptr<native_transform>` cache member in the public header
		// without pulling in anything raylib-specific.
		struct Transform {
			float x = 0.f, y = 0.f;
			float ox = 0.f, oy = 0.f;
			float angle = 0.f;
			float scaleX = 1.f, scaleY = 1.f;
		};

		// Native event for the raylib backend. Raylib has no unified sf::Event
		// analogue — its input API is polled state (IsKeyDown, GetKeyPressed,
		// IsMouseButtonDown, ...). RaylibEventQueue.cpp synthesizes a queue
		// by diffing per-frame state and pushes these tagged PODs; the
		// `event_as_*` Ops read the appropriate payload field based on `type`.
		//
		// Plain struct (not a union) — payload structs are ~16 bytes each,
		// total footprint ~100 bytes. Keeps `raylib::Event` trivially copyable
		// and aggregate-constructible, avoiding the union-with-non-trivial-
		// members dance that cgss::* payloads' default-initializers would
		// otherwise require.
		struct Event {
			cgss::EventType type = cgss::EventType::Closed;
			cgss::KeyEvent            key;
			cgss::TextEvent           text;
			cgss::SizeEvent           size;
			cgss::MouseMoveEvent      mouseMove;
			cgss::MouseButtonEvent    mouseButton;
			cgss::MouseWheelEvent     mouseWheel;
			cgss::GamepadButtonEvent  gamepadButton;
			cgss::GamepadMoveEvent    gamepadMove;
			cgss::GamepadConnectEvent gamepadConnect;
			cgss::TouchEvent          touch;
			cgss::SensorEvent         sensor;
		};
	}

	struct RaylibBackend {
		// ---- Native typedefs ----
		using native_render_target  = ::RenderTexture;
		using native_texture        = ::Texture;
		using native_image          = ::Image;
		using native_sprite         = raylib::Sprite;
		using native_view           = raylib::View;
		using native_render_states  = raylib::RenderStates;
		using native_font           = ::Font;
		using native_event          = raylib::Event;
		using native_window         = raylib::Window;
		using native_render_texture = ::RenderTexture;
		using native_shader         = ::Shader;
		using native_vertex_array   = ::Mesh;
		using native_transform      = raylib::Transform;

		// ---- Capability tags ----
		static constexpr bool has_window      = true;
		static constexpr bool has_event_pump  = true;
		static constexpr bool owns_gpu_memory = true;
		// Raylib has no `DisplayWindowSnapshot` impl yet — DisplayWindow falls
		// back to `StubDisplayWindowSnapshot<...>` via tag-based dispatch.
		static constexpr bool has_snapshot    = false;
		// Raylib uses global window state (InitWindow/CloseWindow are free
		// functions, `native_window` is an empty placeholder). Instance-only
		// ops (window_set_active, window_poll_event, window_wait_event,
		// window_get_default_view, window_set_view, window_get_settings) stay
		// as no-op stubs for Ops symmetry; callers must gate on this tag.
		static constexpr bool window_is_stateful = false;

		// Input capability tags — see SFMLBackendFwd.h for rationale.
		// Raylib exposes all four input families (keyboard, mouse, gamepad,
		// touch) as polled state + has a sync mouse-wheel delta query.
		static constexpr bool has_input_keyboard    = true;
		static constexpr bool has_input_mouse       = true;
		static constexpr bool has_input_gamepad     = true;
		static constexpr bool has_input_touch       = true;
		static constexpr bool has_wheel_state_query = true;

		// 3D capability tags — see SFMLBackendFwd.h. Raylib does ship basic
		// 3D primitives (Camera3D, Mesh, DrawMesh, depth buffer via
		// LoadRenderTextureDepthTex) but cgss's current wrappers don't
		// surface them — tags stay false until the matching cgss:: public
		// API lands. Future raylib specializations that use those paths
		// should flip the relevant tag and gate their code with it.
		static constexpr bool supports_3d                 = false;
		static constexpr bool supports_depth_buffer       = false;
		static constexpr bool supports_perspective_camera = false;
		static constexpr bool supports_mesh_primitive     = false;
		static constexpr bool supports_compute_shader     = false;
		static constexpr bool supports_instanced_draw     = false;

		struct Ops;
	};

}

#endif
