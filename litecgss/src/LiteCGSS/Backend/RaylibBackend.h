#ifndef CGSS_BACKEND_RAYLIB_BACKEND_H
#define CGSS_BACKEND_RAYLIB_BACKEND_H

// Raylib backend — scaffold only (PR M scope b).
//
// Defines `RaylibBackend::Ops` so that wrappers whose bodies route exclusively
// through `B::Ops::*` (Camera, DrawState, Font, Event, RenderTarget) can
// instantiate under `CGSS_BACKEND=raylib`. Texture / Image / Sprite / Shape /
// DisplayWindow currently FAIL to instantiate — by design, to expose where
// further work is required:
//
//   * Texture/Image: `create(filename)` calls `TextureFileSerializeMethod` /
//     `ImageFileSerializeMethod`, both SFML-typed (PR K deferred templating
//     of the SerializeMethod structs themselves).
//   * Sprite/Shape: bodies build `sf::RenderStates` and call `.draw(sprite,
//     states)` on the native render target — SFML-shaped APIs with no
//     drop-in raylib equivalent.
//   * DisplayWindow: body is fully SFML-coupled (PR J); needs a full
//     `basic_DisplayWindow<RaylibBackend>` specialization.
//
// Op bodies below are intentionally stubs (no raylib lib linked yet). The
// goal is to validate the abstraction's *shape* — that backend selection
// flips through CMake without changing public-API names — not to render
// anything.

#include "LiteCGSS/Backend/RaylibBackendFwd.h"
#include "LiteCGSS/Backend/Internal/RaylibEventQueue.h"
#include "LiteCGSS/Backend/Internal/RaylibKeyTranslation.h"

#include <cstdint>
#include <cstddef>
#include <optional>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Events/EventPayloads.h"
#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Transformable.h"

#include <string>

namespace cgss::backend {

	struct RaylibBackend::Ops {
		// ---- Wrapper native accessors (same shape as SFMLBackend::Ops) ----
		template <class W>
		static inline auto& native_of(W& w) noexcept { return w.native(); }
		template <class W>
		static inline const auto& native_of(const W& w) noexcept { return w.native(); }

		// Shared / weak access to a Texture's native backing — see
		// SFMLBackend.h for rationale. Replaces the removed public
		// basic_Texture::ref() / weak() escape hatch.
		template <class B2>
		static inline std::shared_ptr<typename B2::native_texture>
		texture_share(cgss::basic_Texture<B2>& t) noexcept {
			return t.m_texture;
		}
		template <class B2>
		static inline std::weak_ptr<typename B2::native_texture>
		texture_weak(cgss::basic_Texture<B2>& t) noexcept {
			return t.m_texture;
		}

		// ---- Transform composition ----
		// Placeholder — raylib has no sf::Transform analogue (its draw functions
		// take position/rotation/scale as separate args). The native_transform
		// POD (`raylib::Transform` in the fwd header) is populated here so the
		// Sprite/Shape item cache path compiles uniformly across backends.
		static inline native_transform build_transform(const cgss::Transformable& xf) noexcept {
			return native_transform{ xf.getX(), xf.getY(), xf.getOx(), xf.getOy(), xf.getAngle(), xf.getScaleX(), xf.getScaleY() };
		}

		// ---- Window ops — raylib uses GLOBAL window state ----
		// raylib's `raylib::Window` placeholder is empty; all methods below
		// ignore the reference argument and call raylib's free functions
		// against the single process-global window. `window_is_stateful =
		// false` on the backend Fwd signals this to any consumer that cares.
		//
		// Close latching: raylib's WindowShouldClose() returns true once the
		// user clicks the X or presses Esc (with default config). We AND
		// together that signal with an explicit `window_close()` call via a
		// process-global flag in raylib_detail — matches SFML's sticky
		// `!w.isOpen()` semantics.
		static inline void window_close(native_window&) {
			if (IsWindowReady()) CloseWindow();
			raylib_detail::markWindowClosed();
		}
		static inline bool window_is_open(const native_window&) {
			if (!IsWindowReady()) return false;
			if (raylib_detail::isWindowClosed()) return false;
			return !WindowShouldClose();
		}
		// Raylib's draw-frame protocol is BeginDrawing + ClearBackground
		// ... user draws ... EndDrawing. Map cgss's clear/display pair onto
		// Begin+Clear / EndDrawing so the ext/ Ruby Window.update/present
		// lifecycle keeps its existing shape.
		// `::Color{}` fully-qualifies raylib's global Color — plain `Color`
		// here resolves to `cgss::Color` via enclosing namespace lookup.
		static inline void window_clear(native_window&) {
			BeginDrawing();
			ClearBackground(::Color{ 0, 0, 0, 255 });
		}
		static inline void window_display(native_window&) {
			EndDrawing();
		}
		static inline void window_set_title(native_window&, const char* title) {
			SetWindowTitle(title);
		}
		static inline void window_set_size(native_window&, unsigned int w, unsigned int h) {
			SetWindowSize(static_cast<int>(w), static_cast<int>(h));
		}
		static inline Size2u window_get_size(const native_window&) noexcept {
			return { static_cast<unsigned int>(GetScreenWidth()),
			         static_cast<unsigned int>(GetScreenHeight()) };
		}
		static inline void window_set_mouse_cursor_visible(native_window&, bool visible) {
			if (visible) ShowCursor(); else HideCursor();
		}
		// Vsync under raylib: FLAG_VSYNC_HINT is a pre-InitWindow config
		// flag. Post-init we best-effort fall back to SetTargetFPS (matching
		// the previous raylib-direct Ruby Window.set_vsync behaviour).
		static inline void window_set_vsync(native_window&, bool enable) {
			if (!IsWindowReady()) {
				if (enable) SetConfigFlags(FLAG_VSYNC_HINT);
			} else {
				SetTargetFPS(enable ? 60 : 0);
			}
		}
		static inline void window_set_framerate_limit(native_window&, unsigned int fps) {
			SetTargetFPS(static_cast<int>(fps));
		}
		static inline void window_set_position(native_window&, int x, int y) {
			SetWindowPosition(x, y);
		}
		static inline Point2i window_get_position(const native_window&) noexcept {
			const auto p = GetWindowPosition();
			return { static_cast<int>(p.x), static_cast<int>(p.y) };
		}
		static inline Size2u desktop_size() noexcept {
			const int m = GetCurrentMonitor();
			return { static_cast<unsigned int>(GetMonitorWidth(m)),
			         static_cast<unsigned int>(GetMonitorHeight(m)) };
		}
		// Raw-pixel icon setter. Raylib's SetWindowIcon takes a raylib::Image;
		// build one from the pixel buffer, push, destroy.
		static inline void window_set_icon(native_window&, unsigned int width, unsigned int height, const std::uint8_t* pixels) {
			::Image img{};
			// Const-cast is safe: SetWindowIcon copies the pixel data.
			img.data    = const_cast<std::uint8_t*>(pixels);
			img.width   = static_cast<int>(width);
			img.height  = static_cast<int>(height);
			img.mipmaps = 1;
			img.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			SetWindowIcon(img);
		}
		// File-path icon setter — each backend owns the decoder so ext/
		// doesn't need to drag in a format-specific library.
		static inline void window_set_icon_from_file(native_window&, const char* path) {
			::Image img = LoadImage(path);
			if (img.data == nullptr) return;
			SetWindowIcon(img);
			UnloadImage(img);
		}

		// ---- Instance-only window ops (SFML model only) ----
		// raylib has no equivalent concept: context-activate, per-event pump
		// (raylib polls inputs internally via IsKeyPressed / GetGesture), and
		// window-embedded views (raylib routes views through render-targets).
		// Kept as no-op stubs for BackendConcept symmetry; callers must gate
		// on `RaylibBackend::window_is_stateful` (= false) and avoid these.
		static inline void window_set_active(native_window&, bool) {}
		// Drain one synthesized event from the raylib event queue. See
		// Backend/Internal/RaylibEventQueue.cpp for the state-diffing logic
		// that produces these events from raylib's polled-only API.
		static inline bool window_poll_event(native_window&, native_event& ev) {
			return raylib_detail::tryPopEvent(ev);
		}
		// raylib has no blocking-wait primitive; fall back to the poll form.
		static inline bool window_wait_event(native_window&, native_event& ev) {
			return raylib_detail::tryPopEvent(ev);
		}

		// ---- Event translation ops ----
		// native_event (raylib::Event) is already populated by the synthetic
		// queue with typed payloads — just read the matching field based on
		// the event type tag.
		static inline cgss::EventType event_type(const native_event& e) noexcept { return e.type; }

		static inline std::optional<cgss::KeyEvent> event_as_key_press(const native_event& e) {
			if (e.type != cgss::EventType::KeyPressed) return std::nullopt;
			return e.key;
		}
		static inline std::optional<cgss::KeyEvent> event_as_key_release(const native_event& e) {
			if (e.type != cgss::EventType::KeyReleased) return std::nullopt;
			return e.key;
		}
		static inline std::optional<cgss::TextEvent> event_as_text_entered(const native_event& e) {
			if (e.type != cgss::EventType::TextEntered) return std::nullopt;
			return e.text;
		}
		static inline std::optional<cgss::SizeEvent> event_as_resize(const native_event& e) {
			if (e.type != cgss::EventType::Resized) return std::nullopt;
			return e.size;
		}
		static inline std::optional<cgss::MouseMoveEvent> event_as_mouse_move(const native_event& e) {
			if (e.type != cgss::EventType::MouseMoved) return std::nullopt;
			return e.mouseMove;
		}
		static inline std::optional<cgss::MouseButtonEvent> event_as_mouse_button_press(const native_event& e) {
			if (e.type != cgss::EventType::MouseButtonPressed) return std::nullopt;
			return e.mouseButton;
		}
		static inline std::optional<cgss::MouseButtonEvent> event_as_mouse_button_release(const native_event& e) {
			if (e.type != cgss::EventType::MouseButtonReleased) return std::nullopt;
			return e.mouseButton;
		}
		static inline std::optional<cgss::MouseWheelEvent> event_as_mouse_wheel(const native_event& e) {
			if (e.type != cgss::EventType::MouseWheelScrolled) return std::nullopt;
			return e.mouseWheel;
		}
		static inline std::optional<cgss::GamepadButtonEvent> event_as_gamepad_button_press(const native_event& e) {
			if (e.type != cgss::EventType::JoystickButtonPressed) return std::nullopt;
			return e.gamepadButton;
		}
		static inline std::optional<cgss::GamepadButtonEvent> event_as_gamepad_button_release(const native_event& e) {
			if (e.type != cgss::EventType::JoystickButtonReleased) return std::nullopt;
			return e.gamepadButton;
		}
		static inline std::optional<cgss::GamepadMoveEvent> event_as_gamepad_move(const native_event& e) {
			if (e.type != cgss::EventType::JoystickMoved) return std::nullopt;
			return e.gamepadMove;
		}
		static inline std::optional<cgss::GamepadConnectEvent> event_as_gamepad_connect(const native_event& e) {
			if (e.type != cgss::EventType::JoystickConnected) return std::nullopt;
			return e.gamepadConnect;
		}
		static inline std::optional<cgss::GamepadConnectEvent> event_as_gamepad_disconnect(const native_event& e) {
			if (e.type != cgss::EventType::JoystickDisconnected) return std::nullopt;
			return e.gamepadConnect;
		}
		static inline std::optional<cgss::TouchEvent> event_as_touch(const native_event& e) {
			switch (e.type) {
				case cgss::EventType::TouchBegan:
				case cgss::EventType::TouchMoved:
				case cgss::EventType::TouchEnded:
					return e.touch;
				default:
					return std::nullopt;
			}
		}
		// Sensor: raylib has no sensor API. Always nullopt.
		static inline std::optional<cgss::SensorEvent> event_as_sensor(const native_event&) { return std::nullopt; }

		// ---- Input ops (synchronous state queries) ----

		static inline bool keyboard_is_key_pressed(cgss::KeyCode code) {
			const int k = raylib_detail::toRaylibKey(code);
			if (k == KEY_NULL) return false;
			return IsKeyDown(k);
		}

		static inline bool mouse_is_button_pressed(cgss::MouseButton btn) {
			const int i = static_cast<int>(btn);
			// VerticalWheel/HorizontalWheel are cgss sentinels (10, 11) —
			// not raylib buttons. Range-gate against raylib's 0..6 buttons.
			if (i < 0 || i > MOUSE_BUTTON_BACK) return false;
			return IsMouseButtonDown(i);
		}
		static inline Point2i mouse_get_position() {
			const auto p = GetMousePosition();
			return { static_cast<int>(p.x), static_cast<int>(p.y) };
		}
		static inline Point2i mouse_get_position(const native_window&) {
			// raylib has no window-relative mouse query — global is the same
			// thing (raylib only owns one window).
			return mouse_get_position();
		}
		static inline void mouse_set_position(int x, int y) {
			SetMousePosition(x, y);
		}
		static inline void mouse_set_position(int x, int y, const native_window&) {
			SetMousePosition(x, y);
		}
		static inline float mouse_wheel_delta() noexcept {
			return GetMouseWheelMove();
		}

		static inline bool gamepad_is_connected(unsigned int id) {
			return IsGamepadAvailable(static_cast<int>(id));
		}
		// cgss::GamepadButton numeric layout matches raylib GAMEPAD_BUTTON_*
		// 1:1 (Unknown=0..RightThumb=17), so static_cast suffices.
		static inline bool gamepad_is_button_pressed(unsigned int id, cgss::GamepadButton btn) {
			return IsGamepadButtonDown(static_cast<int>(id), static_cast<int>(btn));
		}
		static inline bool gamepad_is_button_released(unsigned int id, cgss::GamepadButton btn) {
			return IsGamepadButtonReleased(static_cast<int>(id), static_cast<int>(btn));
		}
		// cgss::GamepadAxis numeric layout matches raylib GAMEPAD_AXIS_* 1:1.
		static inline float gamepad_axis_position(unsigned int id, cgss::GamepadAxis axis) {
			return GetGamepadAxisMovement(static_cast<int>(id), static_cast<int>(axis));
		}
		static inline unsigned int gamepad_axis_count(unsigned int id) {
			const int n = GetGamepadAxisCount(static_cast<int>(id));
			return n < 0 ? 0u : static_cast<unsigned int>(n);
		}
		static inline const char* gamepad_name(unsigned int id) {
			return GetGamepadName(static_cast<int>(id));
		}
		// Vibration: raylib 5.0 (vendored) has no public SetGamepadVibration
		// API — that lands in 5.1+. No-op here; a future raylib bump can
		// wire this up. Matches SFML backend behaviour (also no-op).
		static inline void gamepad_set_vibration(unsigned int /*id*/, float /*left*/, float /*right*/, float /*ms*/) {}
		static inline void gamepad_update() {
			PollInputEvents();
		}

		static inline unsigned int touch_count() noexcept {
			const int n = GetTouchPointCount();
			return n < 0 ? 0u : static_cast<unsigned int>(n);
		}
		static inline Point2i touch_position(unsigned int index) {
			const auto p = GetTouchPosition(static_cast<int>(index));
			return { static_cast<int>(p.x), static_cast<int>(p.y) };
		}
		static inline unsigned int touch_id(unsigned int index) {
			const int id = GetTouchPointId(static_cast<int>(index));
			return id < 0 ? 0u : static_cast<unsigned int>(id);
		}
		static inline native_view window_get_default_view(const native_window&) { return {}; }
		static inline void window_set_view(native_window&, const native_view&) {}
		static inline cgss::DisplayWindowContextSettings window_get_settings(const native_window&) {
			return {};
		}
		// Create the process-global raylib window. Idempotent: if raylib's
		// window is already initialised (e.g. reload path), no-op. Fullscreen
		// and context settings beyond size/title are ignored — raylib has no
		// equivalent of sf::ContextSettings (bits-per-pixel / MSAA / etc. are
		// configured via SetConfigFlags pre-init; callers that need those
		// must set them before window_create via window_set_vsync-style Ops).
		static inline void window_create(native_window&, const WindowCreateOptions& opts, const char* title) {
			if (IsWindowReady()) return;
			SetConfigFlags(FLAG_WINDOW_RESIZABLE);
			if (opts.fullscreen) SetConfigFlags(FLAG_FULLSCREEN_MODE);
			InitWindow(static_cast<int>(opts.width),
			           static_cast<int>(opts.height),
			           title != nullptr ? title : "LiteRGSS");
			raylib_detail::resetWindowClosed();
		}
		// Stub: raylib doesn't have a "window-as-render-target" concept (drawing
		// to the screen is BeginDrawing/EndDrawing, not via a target object).
		// A real impl would return a tagged sentinel and dispatch the draw_*
		// ops on the tag. For now, returns a static placeholder RenderTexture
		// — drawing to it goes nowhere visible.
		static inline native_render_target& target_from_window(native_window&) noexcept {
			static native_render_target placeholder = {};
			return placeholder;
		}
		static inline void window_draw_render_texture(native_window&, const native_render_texture&, const native_render_states*) {}
		static inline void window_draw_color_quad(native_window&, float, float, float, float, cgss::Color) {}

		// ---- Render texture ops (stubs) ----
		static inline bool render_texture_create(native_render_texture&, unsigned int, unsigned int) { return false; }
		// Depth-aware creation stub. Real raylib bodies would use
		// `LoadRenderTexture(w, h)` vs `LoadRenderTextureDepthTex(w, h)`
		// (raylib has both; the Depth variant attaches a depth texture
		// instead of a depth renderbuffer so the depth is sampleable for
		// post-processing). Stays a stub for now — raylib's RenderTexture
		// Ops are not yet implemented beyond this signature for parity.
		static inline bool render_texture_create_with_depth(native_render_texture&, unsigned int, unsigned int, bool) { return false; }
		static inline void render_texture_set_smooth(native_render_texture&, bool) {}
		static inline const native_texture& render_texture_get_texture(const native_render_texture& rt) { return rt.texture; }
		static inline void render_texture_set_view(native_render_texture&, const native_view&) {}
		static inline void render_texture_display(native_render_texture&) {}
		static inline void render_texture_clear(native_render_texture&) {}
		static inline void render_texture_clear_color(native_render_texture&, cgss::Color) {}

		// ---- View ops (stubs) ----
		// raylib's Camera2D models offset/target/rotation/zoom (transform-
		// based), unlike sf::View's rectangular world-space region. A
		// faithful mapping needs auxiliary state per cgss::Camera; for now
		// these stubs no-op so the abstraction compiles.
		static inline Size2f view_get_size(const native_view&) noexcept { return {}; }
		static inline void view_set_size(native_view&, float, float) noexcept {}
		static inline Size2f view_get_center(const native_view& v) noexcept { return { v.target.x, v.target.y }; }
		static inline void view_set_center(native_view& v, float x, float y) noexcept { v.target = { x, y }; }
		static inline Box2f view_get_viewport(const native_view&) noexcept { return {}; }
		static inline void view_set_viewport(native_view&, float, float, float, float) noexcept {}
		static inline void view_reset(native_view&, float, float, float, float) noexcept {}
		static inline void view_zoom(native_view& v, float factor) noexcept { v.zoom *= factor; }
		static inline void view_set_rotation(native_view& v, float angle) noexcept { v.rotation = angle; }

		// ---- Draw-state ops (stubs) ----
		static inline void draw_state_set_shader(native_render_states&, native_shader*) noexcept {}
		static inline void draw_state_set_blend_factor(native_render_states&, BlendFactorSlot, cgss::BlendFactor) noexcept {}
		static inline cgss::BlendFactor draw_state_get_blend_factor(const native_render_states&, BlendFactorSlot) noexcept { return cgss::BlendFactor::Zero; }
		static inline void draw_state_set_blend_equation(native_render_states&, BlendEquationSlot, cgss::BlendEquation) noexcept {}
		static inline cgss::BlendEquation draw_state_get_blend_equation(const native_render_states&, BlendEquationSlot) noexcept { return cgss::BlendEquation::Add; }

		// ---- Font ops (stubs) ----
		// Real raylib equivalents: LoadFontEx(filename, ...) / LoadFontFromMemory(...)
		static inline bool font_load_from_file(native_font&, const char*) { return false; }
		static inline bool font_load_from_memory(native_font&, const void*, std::size_t) { return false; }

		// ---- Shader ops (stubs) ----
		// Real raylib equivalents would call LoadShaderFromMemory /
		// SetShaderValue / GetShaderLocation; geometry shaders are not
		// supported on raylib's GLES2 default — `shader_is_geometry_available`
		// would return false in a real impl too.
		static inline bool shader_is_available()          { return false; }
		static inline bool shader_is_geometry_available() { return false; }

		static inline bool shader_load_from_memory(native_shader&, const std::string&, cgss::ShaderType) { return false; }
		static inline bool shader_load_from_memory(native_shader&, const std::string&, const std::string&) { return false; }
		static inline bool shader_load_from_memory(native_shader&, const std::string&, const std::string&, const std::string&) { return false; }

		static inline void shader_set_uniform(native_shader&, const std::string&, float) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, int)   {}
		static inline void shader_set_uniform(native_shader&, const std::string&, bool)  {}

		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Vec2&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Vec3&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Vec4&) {}

		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Ivec2&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Ivec3&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Ivec4&) {}

		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Bvec2&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Bvec3&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Bvec4&) {}

		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Mat3&) {}
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Mat4&) {}

		static inline void shader_set_uniform(native_shader&, const std::string&, const native_texture&) {}

		static inline void shader_set_uniform_current_texture(native_shader&, const std::string&) {}

		static inline void shader_set_uniform_array(native_shader&, const std::string&, const float*, std::size_t) {}

		// ---- Render-target view ops (stubs) ----
		static inline void target_set_view(native_render_target&, const native_view&) {}
		static inline native_view target_get_view(const native_render_target&) { return {}; }
		static inline native_view target_get_default_view(const native_render_target&) { return {}; }

		// ---- Draw thunks (stubs) ----
		static inline void draw_sprite(native_render_target&, const native_sprite&, const native_render_states&) {}
		static inline void draw_sprite_with_transform(native_render_target&, const native_sprite&, const native_transform&, const native_render_states*) {}
		static inline void draw_shape_with_transform(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const native_transform&, const native_render_states*) {}

		// ---- Sprite ops (stubs) ----
		static inline cgss::Color get_sprite_color(const native_sprite&) noexcept { return cgss::Colors::White; }
		static inline void set_sprite_color(native_sprite&, cgss::Color) noexcept {}
		static inline cgss::IntRect get_sprite_texture_rect(const native_sprite&) noexcept { return {}; }
		static inline void set_sprite_texture_rect(native_sprite&, const cgss::IntRect&) noexcept {}
		static inline void set_sprite_texture(native_sprite&, const native_texture&, bool) noexcept {}
		static inline void sprite_set_position(native_sprite&, float, float) noexcept {}
		static inline void sprite_set_origin(native_sprite&, float, float) noexcept {}
		static inline void sprite_set_scale(native_sprite&, float, float) noexcept {}
		static inline void draw_sprite(native_render_target&, const native_sprite&) {}
		static inline Size2u get_sprite_bound_texture_size(const native_sprite&) noexcept { return { 0u, 0u }; }

		// ---- Shape ops (stubs) ----
		static inline void draw_shape(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const native_render_states&) {}
		static inline cgss::Point get_shape_point(const cgss::ShapeGeometry&, std::size_t) { return {}; }
		struct Bounds2f { float width = 0.f; float height = 0.f; };
		static inline Bounds2f get_shape_global_bounds(const cgss::ShapeGeometry&, const cgss::Transformable&) { return {}; }

		// ---- Texture ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			// real impl would call SetTextureFilter(t, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
			(void)t; (void)smooth;
		}

		static inline Size2u get_texture_size(const native_texture& t) noexcept {
			return { static_cast<unsigned int>(t.width), static_cast<unsigned int>(t.height) };
		}

		static inline bool create_texture(native_texture& t, unsigned int w, unsigned int h) {
			// real impl would build via LoadTextureFromImage(GenImageColor(w, h, BLANK))
			(void)t; (void)w; (void)h;
			return false;
		}

		static inline void update_texture_from_image(native_texture& t, const native_image& i) {
			// real impl: UpdateTexture(t, i.data);
			(void)t; (void)i;
		}

		// Real raylib equivalent: UpdateTextureRec(t, {x,y,w,h}, pixels);
		static inline void texture_update_pixels(native_texture&, const std::uint8_t*,
		                                        unsigned int, unsigned int,
		                                        unsigned int, unsigned int) {}

		// ---- Image ops ----
		static inline Size2u get_image_size(const native_image& i) noexcept {
			return { static_cast<unsigned int>(i.width), static_cast<unsigned int>(i.height) };
		}

		static inline void copy_image(native_image& dst, const native_image& src) {
			// raylib-real: dst = ImageCopy(src);
			// stub: shallow POD assign — NOT a deep copy of pixel data
			dst = src;
		}

		static inline void image_set_pixel(native_image& i, unsigned int x, unsigned int y, cgss::Color c) {
			// real: ImageDrawPixel(&i, x, y, {c.r, c.g, c.b, c.a});
			(void)i; (void)x; (void)y; (void)c;
		}

		static inline cgss::Color image_get_pixel(const native_image& i, unsigned int x, unsigned int y) {
			(void)i; (void)x; (void)y;
			return cgss::Colors::Black;
		}

		static inline void image_blit(native_image& dst, unsigned int x, unsigned int y,
		                              const native_image& src, const cgss::IntRect& srcRect,
		                              bool applyAlpha = false) {
			(void)dst; (void)x; (void)y; (void)src; (void)srcRect; (void)applyAlpha;
		}

		static inline const std::uint8_t* get_image_pixels_ptr(const native_image& i) noexcept {
			return reinterpret_cast<const std::uint8_t*>(i.data);
		}

		static inline void image_create_with_pixels(native_image& i, unsigned int w, unsigned int h, const std::uint8_t* pixels) {
			(void)i; (void)w; (void)h; (void)pixels;
		}

		static inline void image_create_mask_from_color(native_image& i, cgss::Color color, std::uint8_t alpha) {
			(void)i; (void)color; (void)alpha;
		}
	};

	// Free-function helper mirroring the SFMLBackend equivalent — lets impl
	// .cpps write `cgss::backend::native_of(w)` / `native_of(w)` (ADL).
	template <class W>
	inline auto& native_of(W& w) noexcept {
		return RaylibBackend::Ops::native_of(w);
	}
	template <class W>
	inline const auto& native_of(const W& w) noexcept {
		return RaylibBackend::Ops::native_of(w);
	}

}

#endif
