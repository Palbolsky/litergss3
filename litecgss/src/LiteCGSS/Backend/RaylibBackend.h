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

// rlgl.h — raylib's immediate-mode matrix stack (rlPushMatrix/rlTranslatef/
// rlScalef). Used by viewport_begin/end Ops. Lives alongside raylib.h on
// any real raylib install; minimal submodule checkouts may skip it.
// When absent, forward-declare the four functions we use so the header
// stays compilable in stripped-down environments (CI image build-tests,
// etc.). A real build links against libraylib and resolves the symbols.
#if __has_include(<rlgl.h>)
#  include <rlgl.h>
#else
extern "C" {
	void rlPushMatrix(void);
	void rlPopMatrix(void);
	void rlTranslatef(float x, float y, float z);
	void rlScalef(float x, float y, float z);
}
#endif

#include <cstdint>
#include <cstddef>
#include <cstring>
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
		//
		// Pre-InitWindow guard rationale: every window_* op below dispatches
		// to a raylib entrypoint that ultimately reaches glfw with the
		// process-global window handle. If InitWindow hasn't run, that handle
		// is NULL and glfw's debug asserts fire. SFML's equivalent setters on
		// a not-yet-created sf::RenderWindow are silent no-ops, so gating
		// here matches the SFML contract and makes the cgss::DisplayWindow
		// API backend-uniform.
		static inline void window_clear(native_window&) {
			if (!IsWindowReady()) return;
			BeginDrawing();
			ClearBackground(::Color{ 0, 0, 0, 255 });
		}
		static inline void window_display(native_window&) {
			if (!IsWindowReady()) return;
			// Close any scissor region installed by target_set_view during
			// the frame so the next frame starts from a clean GL state.
			raylib_detail::endCurrentScissor();
			EndDrawing();
			// Clear the once-per-frame refill latch so the next frame's
			// event drain polls raylib's freshly-updated input state.
			raylib_detail::markFrameBoundary();
		}
		static inline void window_set_title(native_window&, const char* title) {
			if (!IsWindowReady()) return;
			SetWindowTitle(title);
		}
		static inline void window_set_size(native_window&, unsigned int w, unsigned int h) {
			if (!IsWindowReady()) return;
			SetWindowSize(static_cast<int>(w), static_cast<int>(h));
		}
		static inline Size2u window_get_size(const native_window&) noexcept {
			// GetScreenWidth/Height read CORE globals (no glfw call) and
			// return 0 pre-InitWindow — safe without a gate.
			return { static_cast<unsigned int>(GetScreenWidth()),
			         static_cast<unsigned int>(GetScreenHeight()) };
		}
		static inline void window_set_mouse_cursor_visible(native_window&, bool visible) {
			if (!IsWindowReady()) return;
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
		// Both map to glfwSetWindowPos / glfwGetWindowPos which assert on a
		// null window handle. Match SFML's "no-op before creation" semantics
		// by gating on IsWindowReady() — same pattern used for window_close /
		// window_is_open above.
		static inline void window_set_position(native_window&, int x, int y) {
			if (!IsWindowReady()) return;
			SetWindowPosition(x, y);
		}
		static inline Point2i window_get_position(const native_window&) noexcept {
			if (!IsWindowReady()) return { 0, 0 };
			const auto p = GetWindowPosition();
			return { static_cast<int>(p.x), static_cast<int>(p.y) };
		}
		// Raylib's GetCurrentMonitor / GetMonitorWidth route through glfw's
		// monitor API, which requires an initialized glfw context (i.e. an
		// InitWindow-created window). Pre-window calls produce "GLFW: Failed
		// to find selected monitor" warnings and return 0. Return {0, 0}
		// explicitly so callers can detect the unavailable state without the
		// console noise.
		//
		// Known contract divergence from SFML: sf::VideoMode::getDesktopMode()
		// is a global OS query that works any time; the raylib backend can
		// only answer this after the window is created. Callers that need
		// pre-window monitor info (to e.g. size the window to a fraction of
		// the desktop) must guard on the returned zero dims.
		static inline Size2u desktop_size() noexcept {
			if (!IsWindowReady()) return { 0u, 0u };
			const int m = GetCurrentMonitor();
			return { static_cast<unsigned int>(GetMonitorWidth(m)),
			         static_cast<unsigned int>(GetMonitorHeight(m)) };
		}
		// Raw-pixel icon setter. Raylib's SetWindowIcon takes a raylib::Image;
		// build one from the pixel buffer, push, destroy.
		static inline void window_set_icon(native_window&, unsigned int width, unsigned int height, const std::uint8_t* pixels) {
			if (!IsWindowReady()) return;
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
			if (!IsWindowReady()) return;
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
		static inline native_view window_get_default_view(const native_window&) {
			// Default view: full-screen viewport, world-space window matching
			// the native pixel dimensions, origin at top-left (center at half).
			const float w = IsWindowReady() ? static_cast<float>(GetScreenWidth())  : 0.f;
			const float h = IsWindowReady() ? static_cast<float>(GetScreenHeight()) : 0.f;
			return native_view{ w * 0.5f, h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f, 0.f };
		}
		static inline void window_set_view(native_window&, const native_view& v) {
			raylib_detail::setCurrentView(v);
		}
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
		static inline void render_texture_set_view(native_render_texture&, const native_view& v) {
			raylib_detail::setCurrentView(v);
		}
		static inline void render_texture_display(native_render_texture&) {}
		static inline void render_texture_clear(native_render_texture&) {}
		static inline void render_texture_clear_color(native_render_texture&, cgss::Color) {}

		// ---- View ops ----
		// raylib::View stores sf::View-compatible state (center / size /
		// normalized viewport / rotation). The effective Camera2D-style
		// transform is computed on-demand in draw_sprite_impl from this
		// state + the current GetScreenWidth/Height.
		static inline Size2f view_get_size(const native_view& v) noexcept { return { v.sizeX, v.sizeY }; }
		static inline void   view_set_size(native_view& v, float w, float h) noexcept { v.sizeX = w; v.sizeY = h; }
		static inline Size2f view_get_center(const native_view& v) noexcept { return { v.cx, v.cy }; }
		static inline void   view_set_center(native_view& v, float x, float y) noexcept { v.cx = x; v.cy = y; }
		static inline Box2f  view_get_viewport(const native_view& v) noexcept { return { v.vpX, v.vpY, v.vpW, v.vpH }; }
		static inline void   view_set_viewport(native_view& v, float x, float y, float w, float h) noexcept {
			v.vpX = x; v.vpY = y; v.vpW = w; v.vpH = h;
		}
		// sf::View::reset(rect): replace center+size with the rect's params.
		static inline void view_reset(native_view& v, float left, float top, float width, float height) noexcept {
			v.cx = left + width  * 0.5f;
			v.cy = top  + height * 0.5f;
			v.sizeX = width;
			v.sizeY = height;
			v.rotation = 0.f;
		}
		// sf::View::zoom(factor): scale the world-space window by `factor`.
		static inline void view_zoom(native_view& v, float factor) noexcept {
			v.sizeX *= factor;
			v.sizeY *= factor;
		}
		static inline void view_set_rotation(native_view& v, float angle) noexcept { v.rotation = angle; }

		// ---- Draw-state ops (stubs) ----
		static inline void draw_state_set_shader(native_render_states&, native_shader*) noexcept {}
		static inline void draw_state_set_blend_factor(native_render_states&, BlendFactorSlot, cgss::BlendFactor) noexcept {}
		static inline cgss::BlendFactor draw_state_get_blend_factor(const native_render_states&, BlendFactorSlot) noexcept { return cgss::BlendFactor::Zero; }
		static inline void draw_state_set_blend_equation(native_render_states&, BlendEquationSlot, cgss::BlendEquation) noexcept {}
		static inline cgss::BlendEquation draw_state_get_blend_equation(const native_render_states&, BlendEquationSlot) noexcept { return cgss::BlendEquation::Add; }

		// ---- Font ops ----
		static inline bool font_load_from_file(native_font& f, const char* filename) {
			if (f.texture.id != 0) UnloadFont(f);
			f = LoadFont(filename);
			// raylib's LoadFont returns a default Font on failure — the
			// atlas texture id == 0 signals that.
			return f.texture.id != 0;
		}
		static inline bool font_load_from_memory(native_font& f, const void* data, std::size_t size) {
			if (f.texture.id != 0) UnloadFont(f);
			// LoadFontFromMemory needs a file-type hint (e.g. ".ttf").
			// We assume .ttf here — callers with other formats should
			// call LoadFontFromMemory directly via the extension SPI.
			f = LoadFontFromMemory(".ttf",
			                       reinterpret_cast<const unsigned char*>(data),
			                       static_cast<int>(size),
			                       32,    // default character size
			                       nullptr, 0);
			return f.texture.id != 0;
		}
		// Release the GPU atlas a raylib Font owns. Safe on zero-initialized.
		static inline void font_destroy(native_font& f) noexcept {
			if (f.texture.id != 0) {
				UnloadFont(f);
				f = native_font{};
			}
		}

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

		// ---- Render-target view ops ----
		// target_set_view is the cross-cutting op the draw pipeline uses to
		// install a view (defview, viewport view, framed view, etc.) for the
		// upcoming draw calls. The raylib impl stashes the view in
		// raylib_detail::currentView() and — if the view has a sub-viewport —
		// opens a matching scissor. Callers restore the previous view by
		// invoking target_set_view again with the saved native_view value.
		static inline void target_set_view(native_render_target&, const native_view& v) {
			raylib_detail::setCurrentView(v);
		}
		static inline native_view target_get_view(const native_render_target&) {
			return raylib_detail::currentView();
		}
		// For a render-texture target, the default view's world-space window
		// matches the texture's extent. We don't know the caller's target
		// here (the `::RenderTexture&` is sometimes the window-placeholder;
		// sometimes a real RT) — use the ::Texture dimensions when valid,
		// else fall back to the screen size.
		static inline native_view target_get_default_view(const native_render_target& t) {
			if (t.texture.id != 0) {
				const float w = static_cast<float>(t.texture.width);
				const float h = static_cast<float>(t.texture.height);
				return native_view{ w * 0.5f, h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f, 0.f };
			}
			const float w = IsWindowReady() ? static_cast<float>(GetScreenWidth())  : 0.f;
			const float h = IsWindowReady() ? static_cast<float>(GetScreenHeight()) : 0.f;
			return native_view{ w * 0.5f, h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f, 0.f };
		}

		// ---- Sprite state helpers ----
		// Internal: resolve the effective source rectangle for a draw —
		// falls back to the full texture extents when the caller never
		// called setTextureRect (textureRect is zero-initialized).
		static inline cgss::IntRect resolve_sprite_src_rect(const native_sprite& s) noexcept {
			if (s.textureRect.width == 0 && s.textureRect.height == 0 && s.texture != nullptr) {
				return cgss::IntRect{
					0, 0,
					static_cast<int>(s.texture->width),
					static_cast<int>(s.texture->height)
				};
			}
			return s.textureRect;
		}

		// Resolve the effective screen-pixel translation + uniform zoom
		// implied by the currently-installed raylib::View. Matches sf::View
		// semantics: a world point (wx, wy) maps to screen pixel
		//     screen = viewport_px_center + (world - view_center) * zoom
		// where viewport_px_center = (vpX + vpW/2) * screenW,
		//       zoom              = (vpW * screenW) / sizeX (uniform fallback).
		struct ViewDerived { float tx = 0.f, ty = 0.f, zoomX = 1.f, zoomY = 1.f, rotation = 0.f; };
		static inline ViewDerived derive_current_view() noexcept {
			const auto& v = raylib_detail::currentView();
			ViewDerived d;
			d.rotation = v.rotation;
			if (v.sizeX == 0.f || v.sizeY == 0.f) return d;
			const float sw = static_cast<float>(IsWindowReady() ? GetScreenWidth()  : 0);
			const float sh = static_cast<float>(IsWindowReady() ? GetScreenHeight() : 0);
			d.zoomX = (v.vpW * sw) / v.sizeX;
			d.zoomY = (v.vpH * sh) / v.sizeY;
			const float vpCenterX = v.vpX * sw + v.vpW * sw * 0.5f;
			const float vpCenterY = v.vpY * sh + v.vpH * sh * 0.5f;
			d.tx = vpCenterX - v.cx * d.zoomX;
			d.ty = vpCenterY - v.cy * d.zoomY;
			return d;
		}

		// Shared DrawTexturePro assembler used by all three draw overloads.
		// Converts cgss::Transformable / sprite-local state into raylib's
		// src/dst/origin/rotation form, applying the currently-installed
		// raylib::View transform (see derive_current_view above).
		//
		// `ox`/`oy` are in SOURCE-pixel coordinates (matching SFML/cgss
		// convention); raylib's origin is post-scale DESTINATION pixels, so
		// we multiply by the effective scale. Negative `srcRect.width`
		// encodes horizontal mirror — raylib's DrawTexturePro honors the
		// sign directly (same as SFML).
		static inline void draw_sprite_impl(const native_sprite& s,
		                                    float posX, float posY,
		                                    float ox, float oy,
		                                    float scaleX, float scaleY,
		                                    float angleDegrees) {
			if (s.texture == nullptr || s.texture->id == 0) return;
			const auto rect = resolve_sprite_src_rect(s);
			const float absW = static_cast<float>(rect.width  < 0 ? -rect.width  : rect.width);
			const float absH = static_cast<float>(rect.height < 0 ? -rect.height : rect.height);
			const auto vd = derive_current_view();
			const float effScaleX = scaleX * vd.zoomX;
			const float effScaleY = scaleY * vd.zoomY;
			::Rectangle src{
				static_cast<float>(rect.left), static_cast<float>(rect.top),
				static_cast<float>(rect.width), static_cast<float>(rect.height) };
			::Rectangle dst{
				vd.tx + posX * vd.zoomX,
				vd.ty + posY * vd.zoomY,
				absW * effScaleX,
				absH * effScaleY };
			::Vector2 origin{ ox * effScaleX, oy * effScaleY };
			DrawTexturePro(*s.texture, src, dst, origin, angleDegrees + vd.rotation,
			               ::Color{ s.color.r, s.color.g, s.color.b, s.color.a });
		}

		// ---- Draw thunks ----
		// Render-states arg is accepted for Ops-symmetry; raylib has no
		// sf::RenderStates analogue (shader / blend-mode / transform
		// composition is handled via BeginShaderMode + blend-mode calls),
		// so states are ignored here. Shader support in the sprite path is
		// tracked separately.
		static inline void draw_sprite(native_render_target&, const native_sprite& s, const native_render_states&) {
			draw_sprite_impl(s, s.x, s.y, s.originX, s.originY, s.scaleX, s.scaleY, 0.f);
		}
		static inline void draw_sprite_with_transform(native_render_target&,
		                                              const native_sprite& s,
		                                              const native_transform& xform,
		                                              const native_render_states* /*base_states*/) {
			// cgss::Transformable angle is CW-positive (matches raylib's
			// DrawTexturePro rotation convention), so pass angle as-is.
			draw_sprite_impl(s, xform.x, xform.y, xform.ox, xform.oy, xform.scaleX, xform.scaleY, xform.angle);
		}
		static inline void draw_shape_with_transform(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const native_transform&, const native_render_states*) {}

		// ---- Sprite ops ----
		static inline cgss::Color get_sprite_color(const native_sprite& s) noexcept { return s.color; }
		static inline void set_sprite_color(native_sprite& s, cgss::Color c) noexcept { s.color = c; }
		static inline cgss::IntRect get_sprite_texture_rect(const native_sprite& s) noexcept {
			return resolve_sprite_src_rect(s);
		}
		static inline void set_sprite_texture_rect(native_sprite& s, const cgss::IntRect& r) noexcept { s.textureRect = r; }
		static inline void set_sprite_texture(native_sprite& s, const native_texture& t, bool resetRect) noexcept {
			s.texture = &t;
			if (resetRect) {
				s.textureRect = cgss::IntRect{ 0, 0, static_cast<int>(t.width), static_cast<int>(t.height) };
			}
		}
		static inline void sprite_set_position(native_sprite& s, float x, float y) noexcept { s.x = x; s.y = y; }
		static inline void sprite_set_origin(native_sprite& s, float ox, float oy) noexcept { s.originX = ox; s.originY = oy; }
		static inline void sprite_set_scale(native_sprite& s, float sx, float sy) noexcept { s.scaleX = sx; s.scaleY = sy; }
		static inline void draw_sprite(native_render_target&, const native_sprite& s) {
			draw_sprite_impl(s, s.x, s.y, s.originX, s.originY, s.scaleX, s.scaleY, 0.f);
		}
		static inline Size2u get_sprite_bound_texture_size(const native_sprite& s) noexcept {
			if (s.texture == nullptr) return { 0u, 0u };
			return { static_cast<unsigned int>(s.texture->width),
			         static_cast<unsigned int>(s.texture->height) };
		}

		// ---- Shape ops (stubs) ----
		static inline void draw_shape(native_render_target&, const cgss::ShapeGeometry&, const native_texture*, const native_render_states&) {}
		static inline cgss::Point get_shape_point(const cgss::ShapeGeometry&, std::size_t) { return {}; }
		struct Bounds2f { float width = 0.f; float height = 0.f; };
		static inline Bounds2f get_shape_global_bounds(const cgss::ShapeGeometry&, const cgss::Transformable&) { return {}; }

		// ---- Texture ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			if (t.id == 0) return;
			SetTextureFilter(t, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
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
			UpdateTexture(t, i.data);
		}

		// Create a GPU-backed texture from a CPU-side image. Releases any
		// existing GPU backing first. Used by ext/Sprite to move bitmap
		// data onto the GPU for drawing.
		static inline void texture_create_from_image(native_texture& t, const native_image& i) {
			if (t.id != 0) UnloadTexture(t);
			t = LoadTextureFromImage(i);
		}

		// Deep-release a texture's GPU backing. Safe on zero-initialized
		// textures (id == 0).
		static inline void texture_destroy(native_texture& t) noexcept {
			if (t.id != 0) {
				UnloadTexture(t);
				t = native_texture{};
			}
		}

		// ---- Viewport scope: scissor + transform push/pop ----
		// RGSS-style Viewport semantics: clip rendering to `clipRect` (in
		// window-pixel coordinates) and translate+scale content within.
		// Paired begin/end calls push/pop raylib's implicit rlGL matrix
		// stack — nesting is allowed up to raylib's rlPushMatrix limit.
		static inline void viewport_begin(native_render_target& /*target*/,
		                                  int clipX, int clipY, int clipW, int clipH,
		                                  float offsetX, float offsetY,
		                                  float scaleX, float scaleY) {
			BeginScissorMode(clipX, clipY, clipW, clipH);
			rlPushMatrix();
			rlTranslatef(offsetX, offsetY, 0.f);
			rlScalef(scaleX, scaleY, 1.f);
		}
		static inline void viewport_end(native_render_target& /*target*/) {
			rlPopMatrix();
			EndScissorMode();
		}

		// Filled rect fill (used by ext/Viewport's begin_draw to show the
		// viewport area). Immediate-mode — draws directly to the current
		// BeginDrawing context.
		static inline void draw_filled_rect(native_render_target& /*target*/,
		                                    int x, int y, int w, int h, cgss::Color c) {
			DrawRectangle(x, y, w, h, ::Color{ c.r, c.g, c.b, c.a });
		}

		// Immediate-mode textured draw with source-rect, destination-rect,
		// origin, rotation and tint — matches the RGSS sprite render model.
		// Target parameter is accepted for Ops-symmetry with SFML (raylib's
		// drawing is implicit on the current BeginDrawing context).
		//
		// `srcRect` may have negative width for horizontal mirror — raylib's
		// DrawTexturePro honors that directly.
		static inline void draw_texture_pro(
				native_render_target& /*target*/,
				const native_texture& tex,
				const cgss::IntRect& srcRect,
				float dstX, float dstY, float dstWidth, float dstHeight,
				float originX, float originY,
				float angleDegrees, cgss::Color tint) {
			::Rectangle src{
				static_cast<float>(srcRect.left),  static_cast<float>(srcRect.top),
				static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			::Rectangle dst{ dstX, dstY, dstWidth, dstHeight };
			::Vector2 origin{ originX, originY };
			DrawTexturePro(tex, src, dst, origin, angleDegrees,
			               ::Color{ tint.r, tint.g, tint.b, tint.a });
		}

		// Update a sub-rectangle of a texture from a raw pixel buffer. The
		// source buffer layout must be R8G8B8A8 at the given stride; raylib
		// handles the upload via UpdateTextureRec.
		static inline void texture_update_pixels(native_texture& t, const std::uint8_t* pixels,
		                                        unsigned int x, unsigned int y,
		                                        unsigned int w, unsigned int h) {
			::Rectangle r{ static_cast<float>(x), static_cast<float>(y),
			               static_cast<float>(w), static_cast<float>(h) };
			UpdateTextureRec(t, r, pixels);
		}

		// ---- Image ops ----
		static inline Size2u get_image_size(const native_image& i) noexcept {
			return { static_cast<unsigned int>(i.width), static_cast<unsigned int>(i.height) };
		}

		// Deep copy pixel data — raylib's ImageCopy allocates a new backing.
		// The destination's existing data (if any) is released first so we
		// don't leak.
		static inline void copy_image(native_image& dst, const native_image& src) {
			if (dst.data != nullptr) UnloadImage(dst);
			dst = ImageCopy(src);
		}

		static inline void image_set_pixel(native_image& i, unsigned int x, unsigned int y, cgss::Color c) {
			ImageDrawPixel(&i, static_cast<int>(x), static_cast<int>(y),
			               ::Color{ c.r, c.g, c.b, c.a });
		}

		static inline cgss::Color image_get_pixel(const native_image& i, unsigned int x, unsigned int y) {
			const ::Color c = GetImageColor(i, static_cast<int>(x), static_cast<int>(y));
			return cgss::Color{ c.r, c.g, c.b, c.a };
		}

		static inline void image_blit(native_image& dst, unsigned int x, unsigned int y,
		                              const native_image& src, const cgss::IntRect& srcRect,
		                              bool applyAlpha = false) {
			(void)applyAlpha; // raylib's ImageDraw always alpha-blends.
			::Rectangle sr{ static_cast<float>(srcRect.left),  static_cast<float>(srcRect.top),
			                static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			::Rectangle dr{ static_cast<float>(x),             static_cast<float>(y),
			                static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			// Fully-qualified ::Color{} to avoid the raylib WHITE macro
			// colliding with `cgss::Color` via namespace lookup.
			ImageDraw(&dst, src, sr, dr, ::Color{ 255, 255, 255, 255 });
		}

		static inline const std::uint8_t* get_image_pixels_ptr(const native_image& i) noexcept {
			return reinterpret_cast<const std::uint8_t*>(i.data);
		}

		// Create a raylib Image from a raw R8G8B8A8 buffer. Releases any
		// existing image data first. The input buffer is copied — raylib
		// takes ownership of a fresh allocation.
		static inline void image_create_with_pixels(native_image& i, unsigned int w, unsigned int h, const std::uint8_t* pixels) {
			if (i.data != nullptr) UnloadImage(i);
			::Image tmp{};
			// raylib has no direct "create-from-pixels" function that matches
			// our R8G8B8A8 contract. Use GenImageColor to get a sized image
			// in the right format, then memcpy the pixel data in.
			// `::Color{0,0,0,0}` — raylib's BLANK macro expands to
			// `Color{...}` which resolves to cgss::Color inside this
			// namespace; fully qualify to force the raylib type.
			tmp = GenImageColor(static_cast<int>(w), static_cast<int>(h), ::Color{ 0, 0, 0, 0 });
			// GenImageColor returns PIXELFORMAT_UNCOMPRESSED_R8G8B8A8.
			const std::size_t byteCount = static_cast<std::size_t>(w) * h * 4u;
			std::memcpy(tmp.data, pixels, byteCount);
			i = tmp;
		}

		// Set alpha of all pixels matching `color` (RGB-exact) to the given
		// alpha. Raylib has no direct equivalent; iterate pixels.
		static inline void image_create_mask_from_color(native_image& i, cgss::Color color, std::uint8_t alpha) {
			for (int y = 0; y < i.height; ++y) {
				for (int x = 0; x < i.width; ++x) {
					const ::Color p = GetImageColor(i, x, y);
					if (p.r == color.r && p.g == color.g && p.b == color.b) {
						ImageDrawPixel(&i, x, y, ::Color{ p.r, p.g, p.b, alpha });
					}
				}
			}
		}

		// Deep-free image backing. basic_Image's unique_ptr<native_image>
		// deleter calls raylib::UnloadImage when the pod's data pointer is
		// non-null. Exposed as an Op so the wrapper destructor can invoke
		// it without needing raylib symbols in its public header.
		static inline void image_destroy(native_image& i) noexcept {
			if (i.data != nullptr) {
				UnloadImage(i);
				i = native_image{};
			}
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
