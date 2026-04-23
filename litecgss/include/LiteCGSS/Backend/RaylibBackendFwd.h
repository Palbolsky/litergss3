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

#include <raylib.h>

namespace cgss::backend {

	namespace raylib {
		struct Window {};
		struct Sprite {};
		struct Event {};
		struct RenderStates {};
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
	}

	struct RaylibBackend {
		// ---- Native typedefs ----
		using native_render_target  = ::RenderTexture;
		using native_texture        = ::Texture;
		using native_image          = ::Image;
		using native_sprite         = raylib::Sprite;
		using native_view           = ::Camera2D;
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

		struct Ops;
	};

}

#endif
