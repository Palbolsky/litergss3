#ifndef CGSS_BACKEND_RAYLIB_BACKEND_H
#define CGSS_BACKEND_RAYLIB_BACKEND_H

// Raylib backend — full Op implementation.
//
// Defines `RaylibBackend::Ops` for the complete BackendConcept surface:
// Texture / Image / Sprite / Shape / Text / RenderTarget / RenderTexture /
// View / Window / Event. Drawable bodies route through these Ops (DrawTexturePro,
// rlgl matrix stack, BeginTextureMode/EndTextureMode) so basic_Sprite<B>,
// basic_Shape<B>, basic_DisplayWindow<B> instantiate cleanly under
// CGSS_BACKEND=raylib alongside the SFML backend.
//
// Capability gaps tracked via tag dispatch on RaylibBackendFwd.h:
//   - has_snapshot = false: full freeze/transition machinery isn't ported
//     (basic_DisplayWindow<B>::Snapshot resolves to StubDisplayWindowSnapshot,
//     whose `takeSnapshot` delegates to `cgss::detail::backendCaptureScreen`
//     in DisplayWindowSnapshot.cpp — basic screen captures still work).
// Backend-shaped pieces that get full per-backend folder specializations
// rather than living in this Ops bag: cgss::FramedView's 9-slice skin,
// cgss::ScrollText, cgss::ViewportItem::takeSnapshot. See
// Backend/Specializations/raylib/ for those.

#include "LiteCGSS/Backend/RaylibBackendFwd.h"
#include "Backend/Internal/RaylibEventQueue.h"
#include "Backend/Internal/RaylibFontAtlas.h"
#include "Backend/Internal/RaylibPlatformQueries.h"
#include "Backend/Internal/RaylibShaderTextures.h"
#include "Backend/Internal/RaylibWindowStack.h"
#include "Backend/Internal/RaylibKeyTranslation.h"
#include "Backend/Shader/ShaderPreprocessor.h"
#include "Views/GLContext.h"

// raylib's own headers, from the raylib this build links (see the raylib
// lookup in litecgss/CMakeLists.txt): rlgl.h — the immediate-mode matrix stack
// and blend factors the viewport / blend Ops use — ships with every raylib.
// raylib.h must precede rlgl.h: it defines RL_MATRIX_TYPE so rlgl.h does not
// redefine Matrix.
#include <raylib.h>
#include <rlgl.h>
// Android, hosted surface (raylib built PLATFORM=AndroidHosted, see
// Specializations/raylib/AndroidHostSurface.cpp): the render thread applies
// the host's surface and input when it pumps events, drawing or not.
#if (defined(__ANDROID__) || defined(ANDROID)) && __has_include(<raylib_android_hosted.h>)
#  include <raylib_android_hosted.h>
#  define CGSS_RAYLIB_ANDROID_HOSTED 1
#endif

// The raylib LiteCGSS is written for (its CMake checks the same at configure time).
static_assert(RAYLIB_VERSION_MAJOR >= 6, "LiteCGSS needs raylib 6.0 or newer");

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <vector>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Common/Vector3.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Events/EventPayloads.h"
#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Events/SensorType.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Transformable.h"
#include "Backend/Specializations/raylib/ScrollText.h"

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
		//
		// Angle sign is flipped here to match screen-space Y-down convention,
		// mirroring SFMLBackend::build_transform — see Transformable.h for the
		// cgss-wide convention.
		static inline native_transform build_transform(const cgss::Transformable& xf) noexcept {
			return native_transform{ xf.getX(), xf.getY(), xf.getOx(), xf.getOy(), -xf.getAngle(), xf.getScaleX(), xf.getScaleY() };
		}

		// ---- Window ops — raylib uses GLOBAL window state ----
		// raylib has one process-global window. Each `raylib::Window` is a
		// record stacked on it (Internal/RaylibWindowStack.h): the last one
		// opened owns the screen. The ops below apply to the OS window only
		// for the owner; the others keep their size / title in their record
		// until they own the screen again, and get no draws nor events.
		//
		// Close latching: raylib's WindowShouldClose() returns true once the
		// user clicks the X or presses Esc (with default config). We AND
		// together that signal with an explicit `window_close()` call via a
		// process-global flag in raylib_detail — matches SFML's sticky
		// `!w.isOpen()` semantics.
		//
		// We deliberately do NOT call raylib's CloseWindow() here. CloseWindow
		// tears down the process-global GL context, which invalidates every
		// live raylib Texture/Font the app is still holding. In practice
		// window_close fires mid-event-loop (user presses Esc / clicks X) and
		// the textures that sprites + viewports wrap are released *after* the
		// loop returns, as their owning scopes unwind. Running CloseWindow
		// before those shared_ptr<Texture> deleters fire makes UnloadTexture
		// segfault (rlUnloadTexture → glDeleteTextures with no current
		// context). The actual CloseWindow is registered as an atexit handler
		// in window_create, so it runs after main() returns — by which point
		// every automatic/static texture has already been unloaded.
		static inline bool owns_screen(const native_window& w) noexcept {
			return raylib_detail::screenOwner() == &w;
		}
		// The window below gets the screen back: its size and title.
		static inline void restore_screen(const native_window& w) {
			if (!IsWindowReady()) return;
			SetWindowTitle(w.title.c_str());
			if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && !IsWindowFullscreen()) {
				SetWindowSize(static_cast<int>(w.width), static_cast<int>(w.height));
			}
			// A close that ended the window above (its X button) was its own.
			raylib_detail::resetWindowClosed();
		}
		static inline void window_close(native_window& w) {
			w.last_requested_position.reset();
			const bool wasOwner = owns_screen(w);
			const bool wasOpen = w.open;
			w.open = false;
			if (!wasOpen) {
				if (raylib_detail::screenOwner() == nullptr) raylib_detail::markWindowClosed();
				return;
			}
			raylib::Window* const owner = raylib_detail::windowClosed(w);
			if (owner == nullptr) {
				raylib_detail::markWindowClosed();
			} else if (wasOwner) {
				restore_screen(*owner);
			}
		}
		static inline bool window_is_open(const native_window& w) {
			if (!IsWindowReady() || !w.open) return false;
			// A window waiting under another one is still open.
			if (!owns_screen(w)) return true;
			if (raylib_detail::isWindowClosed()) return false;
			return !WindowShouldClose();
		}
		// Only the screen's owner draws (see the window ops above).
		static inline bool window_is_drawable(const native_window& w) noexcept {
			return owns_screen(w);
		}

		// ---- GL context ----
		// One GL context for every window: whoever does GL work must have it
		// current on its thread (DisplayWindow::AcquireContext).
		static constexpr bool context_follows_thread = true;
		static inline void context_make_current(bool current) {
			raylib_detail::makeContextCurrent(current);
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
		static inline void window_clear(native_window& w) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || !owns_screen(w)) return;
			BeginDrawing();
			apply_default_blend();
			ClearBackground(::Color{ 0, 0, 0, 255 });
		}
		static inline void window_display(native_window& w) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || !owns_screen(w)) return;
			// Close any scissor region installed by target_set_view during
			// the frame so the next frame starts from a clean GL state.
			raylib_detail::endCurrentScissor();
			EndDrawing();
			// Clear the once-per-frame refill latch so the next frame's
			// event drain polls raylib's freshly-updated input state.
			raylib_detail::markFrameBoundary();
		}
		static inline void window_set_title(native_window& win, const char* title) {
			win.title = title != nullptr ? title : "";
			if (!IsWindowReady() || !owns_screen(win)) return;
			SetWindowTitle(win.title.c_str());
		}
		static inline void window_set_size(native_window& win, unsigned int w, unsigned int h) {
			win.width = w;
			win.height = h;
			if (!IsWindowReady() || !owns_screen(win)) return;
			SetWindowSize(static_cast<int>(w), static_cast<int>(h));
		}
		static inline Size2u window_get_size(const native_window& win) noexcept {
			if (!owns_screen(win)) return { win.width, win.height };
			// GetScreenWidth/Height read CORE globals (no glfw call) and
			// return 0 pre-InitWindow — safe without a gate.
			return { static_cast<unsigned int>(GetScreenWidth()),
			         static_cast<unsigned int>(GetScreenHeight()) };
		}
		static inline void window_set_mouse_cursor_visible(native_window& w, bool visible) {
			if (!IsWindowReady() || !owns_screen(w)) return;
			if (visible) ShowCursor(); else HideCursor();
		}
		// Vsync under raylib: FLAG_VSYNC_HINT is latched at InitWindow time
		// (set via WindowCreateOptions.vsync in window_create), so runtime
		// toggling is best-effort. Post-init we fall back to a soft FPS cap
		// matching the previous raylib-direct Ruby Window.set_vsync behaviour
		// — true hardware vsync requires recreating the GL context, which
		// raylib doesn't support. Pre-init callers land here if someone
		// invokes window_set_vsync before window_create; applying the flag is
		// harmless (window_create will re-set it anyway).
		static inline void window_set_vsync(native_window& w, bool enable) {
			if (!IsWindowReady()) {
				if (enable) SetConfigFlags(FLAG_VSYNC_HINT);
				return;
			}
			if (!owns_screen(w)) return;
			SetTargetFPS(enable ? 60 : 0);
		}
		static inline void window_set_framerate_limit(native_window& w, unsigned int fps) {
			if (IsWindowReady() && !owns_screen(w)) return;
			SetTargetFPS(static_cast<int>(fps));
		}
		// Both map to glfwSetWindowPos / glfwGetWindowPos which assert on a
		// null window handle. Match SFML's "no-op before creation" semantics
		// by gating on IsWindowReady() — same pattern used for window_close /
		// window_is_open above.
		//
		// Position get/set pair the cache on `native_window` (see the field
		// doc in RaylibBackendFwd.h): on X11 `glfwSetWindowPos` is async, so
		// a read via `XTranslateCoordinates` right after the write may still
		// return the pre-move coords. Caching on set + preferring the cache
		// on get matches SFMLBackend's synchronous observable behavior.
		static inline void window_set_position(native_window& w, int x, int y) {
			if (!IsWindowReady() || !owns_screen(w)) return;
			w.last_requested_position = Point2i{ x, y };
			SetWindowPosition(x, y);
		}
		static inline Point2i window_get_position(const native_window& w) noexcept {
			if (w.last_requested_position.has_value()) return *w.last_requested_position;
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
		static inline std::vector<cgss::VideoMode> fullscreen_modes() {
			auto modes = raylib_detail::fullscreenModes();
			if (modes.empty()) {
				// Before the window (hosted Android: always), the screen is
				// the one mode, as SFML's Android lists it: games pick their
				// fullscreen resolution from this list before opening the
				// window (PSDK's find_best_matching_resolution).
				const auto screen = desktop_size();
				if (screen.x != 0 && screen.y != 0) {
					modes.push_back({ screen.x, screen.y, 32u });
				}
			}
			return modes;
		}
		static inline unsigned int texture_max_size() {
			cgss::detail::ensureGLContext();
			return raylib_detail::maxTextureSize();
		}
		static inline Size2u desktop_size() noexcept {
#ifdef CGSS_RAYLIB_ANDROID_HOSTED
			// The host's surface is the screen, known before the window
			// (as SFML's hosted Android reports it): games size their
			// window from it (PSDK's fullscreen resolution)
			int surfaceWidth = 0, surfaceHeight = 0;
			if (RaylibHostedGetSurfaceSize(&surfaceWidth, &surfaceHeight)) {
				return { static_cast<unsigned int>(surfaceWidth), static_cast<unsigned int>(surfaceHeight) };
			}
#endif
			if (!IsWindowReady()) return { 0u, 0u };
			const int m = GetCurrentMonitor();
			return { static_cast<unsigned int>(GetMonitorWidth(m)),
			         static_cast<unsigned int>(GetMonitorHeight(m)) };
		}
		// Raw-pixel icon setter. Raylib's SetWindowIcon takes a raylib::Image;
		// build one from the pixel buffer, push, destroy.
		static inline void window_set_icon(native_window& w, unsigned int width, unsigned int height, const std::uint8_t* pixels) {
			if (!IsWindowReady() || !owns_screen(w)) return;
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
		static inline void window_set_icon_from_file(native_window& w, const char* path) {
			if (!IsWindowReady() || !owns_screen(w)) return;
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
		// The events are the screen owner's.
		static inline bool window_poll_event(native_window& w, native_event& ev) {
#ifdef CGSS_RAYLIB_ANDROID_HOSTED
			RaylibHostedPump();
#endif
			return owns_screen(w) && raylib_detail::tryPopEvent(ev);
		}
		// raylib has no blocking-wait primitive; fall back to the poll form.
		static inline bool window_wait_event(native_window& w, native_event& ev) {
#ifdef CGSS_RAYLIB_ANDROID_HOSTED
			RaylibHostedPump();
#endif
			return owns_screen(w) && raylib_detail::tryPopEvent(ev);
		}

		// ---- Event translation ops ----
		// native_event (raylib::Event) is already populated by the synthetic
		// queue with typed payloads — just read the matching field based on
		// the event type tag.
		static inline cgss::EventType event_type(const native_event& e) noexcept { return e.type; }

		// A synthetic key event (cgss::input::injectKey): the same shape the
		// polled-input queue produces.
		static inline void event_set_key(native_event& e, bool pressed, const cgss::KeyEvent& key) {
			e = native_event{};
			e.type = pressed ? cgss::EventType::KeyPressed : cgss::EventType::KeyReleased;
			e.key = key;
		}

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
		}		// raylib keys are physical positions (scancodes); the layout is
		// assumed to be US for the layout conversions.
		static inline bool keyboard_is_scancode_pressed(cgss::Scancode scancode) {
			const int k = raylib_detail::raylibKeyFromScancode(scancode);
			return k != KEY_NULL && IsKeyDown(k);
		}
		static inline cgss::KeyCode keyboard_localize(cgss::Scancode scancode) noexcept {
			const int k = raylib_detail::raylibKeyFromScancode(scancode);
			return k == KEY_NULL ? cgss::KeyCode::Unknown : raylib_detail::fromRaylibKey(k);
		}
		static inline cgss::Scancode keyboard_delocalize(cgss::KeyCode key) noexcept {
			const int k = raylib_detail::toRaylibKey(key);
			return k == KEY_NULL ? cgss::Scancode::Unknown : raylib_detail::scancodeFromRaylibKey(k);
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
			const int pad = static_cast<int>(id);
			// raylib reports the D-pad as buttons: synthesize its axes.
			if (axis == cgss::GamepadAxis::DpadX) {
				return (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ? 1.f : 0.f)
				     - (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ? 1.f : 0.f);
			}
			if (axis == cgss::GamepadAxis::DpadY) {
				return (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_UP) ? 1.f : 0.f)
				     - (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ? 1.f : 0.f);
			}
			return GetGamepadAxisMovement(pad, static_cast<int>(axis));
		}
		static inline unsigned int gamepad_axis_count(unsigned int id) {
			const int n = GetGamepadAxisCount(static_cast<int>(id));
			return n < 0 ? 0u : static_cast<unsigned int>(n);
		}
		static inline const char* gamepad_name(unsigned int id) {
			return GetGamepadName(static_cast<int>(id));
		}
		// Vibration (raylib >= 5.5): both motors, duration in seconds. The SFML
		// backend has none (no-op there).
		static inline void gamepad_set_vibration(unsigned int id, float left, float right, float ms) {
			SetGamepadVibration(static_cast<int>(id), left, right, ms / 1000.f);
		}
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

		// ---- Clipboard ops ----
		static inline std::string clipboard_get_string() {
			const char* text = GetClipboardText();
			return text != nullptr ? std::string(text) : std::string();
		}
		static inline void clipboard_set_string(const std::string& utf8) {
			SetClipboardText(utf8.c_str());
		}

		// ---- Sensor ops ----
		// raylib (6.0) has no device-sensor API: nothing is ever available.
		static inline bool sensor_is_available(cgss::SensorType) noexcept { return false; }
		static inline void sensor_set_enabled(cgss::SensorType, bool) noexcept {}
		static inline cgss::Vector3 sensor_get_value(cgss::SensorType) noexcept { return {}; }
		static inline native_view window_get_default_view(const native_window&) {
			// Default view: full-screen viewport, world-space window matching
			// the native pixel dimensions, origin at top-left (center at half).
			const float w = IsWindowReady() ? static_cast<float>(GetScreenWidth())  : 0.f;
			const float h = IsWindowReady() ? static_cast<float>(GetScreenHeight()) : 0.f;
			return native_view{ w * 0.5f, h * 0.5f, w, h, 0.f, 0.f, 1.f, 1.f, 0.f };
		}
		static inline void window_set_view(native_window&, const native_view& v) {
			cgss::detail::ensureGLContext();
			raylib_detail::setCurrentTargetSize(
				static_cast<float>(IsWindowReady() ? GetScreenWidth()  : 0),
				static_cast<float>(IsWindowReady() ? GetScreenHeight() : 0));
			raylib_detail::setCurrentView(v);
		}
		// raylib's GL profile is fixed when raylib is built: report it.
		static inline cgss::DisplayWindowContextSettings window_get_settings(const native_window&) {
			cgss::DisplayWindowContextSettings settings{};
			switch (rlGetVersion()) {
				case RL_OPENGL_11:    settings.majorVersion = 1; settings.minorVersion = 1; break;
				case RL_OPENGL_21:    settings.majorVersion = 2; settings.minorVersion = 1; break;
				case RL_OPENGL_33:    settings.majorVersion = 3; settings.minorVersion = 3; break;
				case RL_OPENGL_43:    settings.majorVersion = 4; settings.minorVersion = 3; break;
				case RL_OPENGL_ES_20: settings.majorVersion = 2; settings.minorVersion = 0; break;
				default: break;
			}
			return settings;
		}
		// Create — or re-configure — the process-global raylib window. Two
		// code paths fold into this one Op to match SFML's single-call
		// `sf::RenderWindow::create` contract:
		//
		//   * First invocation (IsWindowReady() == false): set the config
		//     flags, InitWindow, and unlatch the close flag.
		//   * Subsequent invocations (reload path — Alt+Enter fullscreen
		//     toggle, resolution change, title change): raylib has no
		//     "recreate" verb (CloseWindow + InitWindow would destroy the
		//     GL context and every texture), so we apply the diff in place.
		//     ToggleBorderlessWindowed is preferred over ToggleFullscreen
		//     because it keeps the monitor's native mode and avoids the
		//     exclusive-fullscreen surface dance — SFML's `sf::Style::
		//     Fullscreen` changes the display mode, raylib's borderless
		//     variant is the closer analogue to what modern users expect
		//     and doesn't glitch on compositing Wayland/X11 sessions.
		static inline void window_create(native_window& win, const WindowCreateOptions& opts, const char* title) {
			cgss::detail::ensureGLContext();
			win.width = opts.width;
			win.height = opts.height;
			win.title = title != nullptr ? title : "LiteRGSS";
			win.open = true;
			// On top of the stack: owns the screen from now on.
			raylib_detail::windowOpened(win);
			if (IsWindowReady()) {
				if (title != nullptr) SetWindowTitle(title);
				// Don't resize while fullscreen — raylib ignores SetWindowSize
				// in fullscreen modes anyway and it would desync on return.
				const bool borderlessNow = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
				const bool exclusiveNow  = IsWindowFullscreen();
				const bool fsNow         = borderlessNow || exclusiveNow;
				if (!fsNow) {
					SetWindowSize(static_cast<int>(opts.width), static_cast<int>(opts.height));
				}
				if (fsNow != opts.fullscreen) {
					// Round-trip exclusive fullscreen back to windowed before
					// switching to borderless — toggling one while the other
					// is active leaves raylib in an inconsistent state.
					if (exclusiveNow) ToggleFullscreen();
					ToggleBorderlessWindowed();
				}
				if (opts.hidden != IsWindowState(FLAG_WINDOW_HIDDEN)) {
					if (opts.hidden) SetWindowState(FLAG_WINDOW_HIDDEN);
					else ClearWindowState(FLAG_WINDOW_HIDDEN);
				}
				// Reload path: a prior window_close() latched g_windowClosed.
				// Clear it so `window_is_open` can see the reused window again.
				raylib_detail::resetWindowClosed();
				return;
			}
			SetConfigFlags(FLAG_WINDOW_RESIZABLE);
			if (opts.fullscreen) SetConfigFlags(FLAG_BORDERLESS_WINDOWED_MODE);
			// FLAG_VSYNC_HINT is latched into the GL context at InitWindow
			// time and cannot be toggled afterwards; this is the only place
			// real hardware vsync can be enabled under raylib.
			if (opts.vsync) SetConfigFlags(FLAG_VSYNC_HINT);
			if (opts.hidden) SetConfigFlags(FLAG_WINDOW_HIDDEN);
			InitWindow(static_cast<int>(opts.width),
			           static_cast<int>(opts.height),
			           title != nullptr ? title : "LiteRGSS");
			// raylib defaults to KEY_ESCAPE as the "exit key", which sets
			// WindowShouldClose() and surfaces as a Closed event in the cgss
			// pipeline. SFML had no such auto-exit, and PSDK maps Escape to
			// the in-game :B virtual button (cancel/back). Disable the
			// hard-coded exit so Escape behaves like any other key.
			SetExitKey(0);
			raylib_detail::resetWindowClosed();
			// Defer CloseWindow to process exit. window_close() only flips the
			// sticky-closed flag, so nothing else tears down raylib's GL
			// context; atexit runs after main() returns, i.e. after all
			// automatic/static shared_ptr<Texture> deleters have unloaded
			// their GPU backings against a still-live context. std::atexit
			// registers once (idempotent for the single-window lifetime
			// raylib enforces; this branch only runs on the first InitWindow).
			std::atexit(+[]() noexcept {
				if (IsWindowReady()) CloseWindow();
			});
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
		// Composite the FBO (the per-frame scene render texture) onto the
		// window, scaling to the screen and applying the optional global
		// shader/blend. Called from DisplayWindow::postProcessing() after
		// EndTextureMode has flushed the FBO. Without this, every frame
		// renders to the FBO and nothing reaches the screen.
		//
		// raylib's FBO color attachment is bottom-up in texel space (OpenGL
		// convention) while our logical content was rendered top-down via
		// BeginTextureMode's flipped projection. To present it upright we
		// pass a source rect with negative height — DrawTexturePro samples
		// from the bottom row up, which inverts back to top-down on screen.
		static inline void window_draw_render_texture(native_window&,
		                                              const native_render_texture& rt,
		                                              const native_render_states* states) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || rt.texture.id == 0) return;
			const float srcW = static_cast<float>(rt.texture.width);
			const float srcH = static_cast<float>(rt.texture.height);
			const float dstW = static_cast<float>(GetScreenWidth());
			const float dstH = static_cast<float>(GetScreenHeight());
			::Rectangle src{ 0.f, srcH, srcW, -srcH };
			::Rectangle dst{ 0.f, 0.f, dstW, dstH };
			::Vector2   origin{ 0.f, 0.f };
			draw_with_render_states(states, [&]() {
				DrawTexturePro(rt.texture, src, dst, origin, 0.f, ::WHITE);
			});
		}
		// Solid-color overlay on the window (used for brightness fade etc.).
		// Bypasses the view transform: draws directly in window pixel space.
		static inline void window_draw_color_quad(native_window&, float x, float y, float w, float h, cgss::Color c) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady()) return;
			DrawRectangle(static_cast<int>(x), static_cast<int>(y),
			              static_cast<int>(w), static_cast<int>(h),
			              ::Color{ c.r, c.g, c.b, c.a });
		}

		// ---- Render texture ops ----
		// Pair semantics matches the SFML RenderTexture lifecycle the cgss
		// Viewport draw flow assumes: clear_color BEGINS texture mode and
		// leaves it active for the subsequent drawables; render_texture_display
		// ENDS it. Callers (cgss::ViewportItem::draw) bracket their draws
		// between these two and must not nest them.
		static inline bool render_texture_create(native_render_texture& rt, unsigned int width, unsigned int height) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || width == 0 || height == 0) return false;
			if (rt.id != 0) UnloadRenderTexture(rt);
			rt = LoadRenderTexture(static_cast<int>(width), static_cast<int>(height));
			return rt.id != 0;
		}
		// Depth-aware creation. raylib's stock LoadRenderTexture already
		// allocates a depth renderbuffer; the `withDepth=false` path is
		// approximated by calling the standard loader (raylib has no built-in
		// "no-depth" RT and the depth attachment is harmless when unused).
		static inline bool render_texture_create_with_depth(native_render_texture& rt, unsigned int width, unsigned int height, bool /*withDepth*/) {
			return render_texture_create(rt, width, height);
		}
		static inline void render_texture_set_smooth(native_render_texture& rt, bool smooth) {
			cgss::detail::ensureGLContext();
			if (rt.texture.id == 0) return;
			SetTextureFilter(rt.texture, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
		}
		static inline const native_texture& render_texture_get_texture(const native_render_texture& rt) { return rt.texture; }
		static inline void render_texture_set_view(native_render_texture& rt, const native_view& v) {
			cgss::detail::ensureGLContext();
			raylib_detail::setCurrentTargetSize(
				static_cast<float>(rt.texture.width),
				static_cast<float>(rt.texture.height));
			raylib_detail::setCurrentView(v);
		}
		// Closes the current BeginTextureMode block opened by
		// render_texture_clear_color. raylib has no separate "display" step
		// (no double-buffer flip on FBOs), so this is just the EndTextureMode
		// pair-closer.
		static inline void render_texture_display(native_render_texture&) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady()) return;
			EndTextureMode();
		}
		// A clear covers the whole texture: glClear honours the scissor test,
		// and the scissor left by the previous view (target_set_view) is in
		// another framebuffer's coordinates. The next view installs its own.
		static inline void render_texture_clear(native_render_texture& rt) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || rt.id == 0) return;
			BeginTextureMode(rt);
			raylib_detail::endCurrentScissor();
			ClearBackground(::Color{ 0, 0, 0, 0 });
		}
		static inline void render_texture_clear_color(native_render_texture& rt, cgss::Color c) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady() || rt.id == 0) return;
			BeginTextureMode(rt);
			raylib_detail::endCurrentScissor();
			ClearBackground(::Color{ c.r, c.g, c.b, c.a });
		}

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

		// ---- Draw-state ops ----
		// Native shader pointer is owned by basic_RenderStatesData<B>; this
		// op just stores the raw pointer into the native render states. The
		// draw_sprite_with_render_states overload below picks it up at draw
		// time and wraps the call in BeginShaderMode/EndShaderMode.
		static inline void draw_state_set_shader(native_render_states& s, native_shader* shader) noexcept {
			s.shader = shader;
		}
		static inline void draw_state_set_blend_factor(native_render_states& s, BlendFactorSlot slot, cgss::BlendFactor f) noexcept {
			switch (slot) {
				case BlendFactorSlot::ColorSrc: s.colorSrc = f; break;
				case BlendFactorSlot::ColorDst: s.colorDst = f; break;
				case BlendFactorSlot::AlphaSrc: s.alphaSrc = f; break;
				case BlendFactorSlot::AlphaDst: s.alphaDst = f; break;
			}
		}
		static inline cgss::BlendFactor draw_state_get_blend_factor(const native_render_states& s, BlendFactorSlot slot) noexcept {
			switch (slot) {
				case BlendFactorSlot::ColorSrc: return s.colorSrc;
				case BlendFactorSlot::ColorDst: return s.colorDst;
				case BlendFactorSlot::AlphaSrc: return s.alphaSrc;
				case BlendFactorSlot::AlphaDst: return s.alphaDst;
			}
			return cgss::BlendFactor::Zero;
		}
		static inline void draw_state_set_blend_equation(native_render_states& s, BlendEquationSlot slot, cgss::BlendEquation e) noexcept {
			switch (slot) {
				case BlendEquationSlot::Color: s.colorEq = e; break;
				case BlendEquationSlot::Alpha: s.alphaEq = e; break;
			}
		}
		static inline cgss::BlendEquation draw_state_get_blend_equation(const native_render_states& s, BlendEquationSlot slot) noexcept {
			switch (slot) {
				case BlendEquationSlot::Color: return s.colorEq;
				case BlendEquationSlot::Alpha: return s.alphaEq;
			}
			return cgss::BlendEquation::Add;
		}

		// ---- Font ops ----
		// LoadFont / LoadFontFromMemory build a GPU atlas (LoadTextureFromImage →
		// rlLoadTexture → glGenTextures) and segfault when called before
		// InitWindow has set up a GL context. Match the "no-op before
		// window_create" pattern used across this backend (window_set_position
		// et al.) and just return false — cgss::Font stays in its unloaded
		// state, non-GPU setters (size / color) remain usable.
		// Fonts are built by raylib_detail::buildFontAtlas, not LoadFontEx:
		// SFML's sizing (character size = em size) and baseline (first one at
		// y = character size), Latin + common symbols, binary alpha for crisp
		// pixel fonts. See Internal/RaylibFontAtlas.h.
		static constexpr int kDefaultFontSize = 32;

		static inline bool font_load_from_memory_with_size(native_font& f, const void* data, std::size_t size, int fontSize) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady()) return false;
			// Glyphs beyond the common ranges are added when texts use them
			return raylib_detail::loadFontAtlas(f, static_cast<const unsigned char*>(data), static_cast<int>(size), fontSize);
		}
		static inline bool font_load_from_file_with_size(native_font& f, const char* filename, int fontSize) {
			if (!IsWindowReady()) return false;
			int dataSize = 0;
			unsigned char* fileData = LoadFileData(filename, &dataSize);
			if (fileData == nullptr) return false;
			const bool loaded = font_load_from_memory_with_size(f, fileData, static_cast<std::size_t>(dataSize), fontSize);
			UnloadFileData(fileData);
			return loaded;
		}
		static inline bool font_load_from_file(native_font& f, const char* filename) {
			return font_load_from_file_with_size(f, filename, kDefaultFontSize);
		}
		static inline bool font_load_from_memory(native_font& f, const void* data, std::size_t size) {
			return font_load_from_memory_with_size(f, data, size, kDefaultFontSize);
		}
		// Release the GPU atlas a raylib Font owns. Safe on zero-initialized.
		// Without a window, the GL context is gone and the atlas texture with
		// it: only the CPU side is freed. A GL call then crashes, at exit
		// above all (fonts in statics, LiteRGSS's font table: the GL library
		// may be unloaded already).
		static inline void font_destroy(native_font& f) noexcept {
			cgss::detail::ensureGLContext();
			raylib_detail::forgetFontAtlas(f);
			if (f.texture.id == 0) return;
			if (IsWindowReady()) {
				UnloadFont(f);
			} else {
				UnloadFontData(f.glyphs, f.glyphCount);
				RL_FREE(f.recs);
			}
			f = native_font{};
		}

		// ---- Shader ops ----
		// Releases a program loaded by shader_load_program, and its uniform
		// locations (a calloc'd array): a defaulted owner deleted the struct
		// without it. raylib's default shader (handed back by a failed load)
		// is never unloaded (UnloadShader skips it). After the window closed
		// no GL call: only the locations go, as font_destroy does.
		static inline void shader_destroy(native_shader& sh) noexcept {
			cgss::detail::ensureGLContext();
			if (sh.id == 0) return;
			raylib_detail::forgetShaderTextures(sh.id);
			if (IsWindowReady()) {
				UnloadShader(sh);
			} else if (sh.locs != nullptr && sh.locs != rlGetShaderLocsDefault()) {
				RL_FREE(sh.locs);
			}
			sh = native_shader{};
		}
		// Clients author shaders against the legacy fixed-pipeline GLSL
		// contract (gl_TexCoord / gl_Color / gl_FragColor / texture2D, a
		// `texture` sampler, gl_Vertex-style vertex shaders), which raylib
		// never feeds. cgss::ShaderPreprocessor rewrites them for the target
		// below before they reach these Ops (see RenderStatesData), and pairs
		// a single stage with a matching default one.
		//
		// Geometry shaders aren't supported on raylib's default GL profile —
		// shader_is_geometry_available() returns false and the 3-arg load
		// rejects accordingly.
		static inline bool shader_is_available()          { return IsWindowReady(); }
		static inline bool shader_is_geometry_available() { return false; }

		// Whether the current GL context accepts the SFML legacy fixed-pipeline
		// GLSL contract (gl_FragColor / gl_TexCoord[0] / texture2D / gl_Color).
		// raylib's profile is baked at compile time of the raylib library —
		// rlGetVersion() returns the active enum without needing a live GL
		// context. RL_OPENGL_11 / RL_OPENGL_21 ship a legacy-compat profile
		// where the keywords work; RL_OPENGL_33 / 43 / ES variants don't.
		// PSDK reads this to set PSDK_SHADER_FIXED_PIPELINE and choose
		// between its legacy and modern shader sources.
		static inline bool shader_supports_fixed_pipeline() {
			const int v = rlGetVersion();
			return v == RL_OPENGL_11 || v == RL_OPENGL_21;
		}

		// Preprocessor target, on raylib's attributes: GLSL 330 core for
		// desktop GL 3.3 / 4.3 builds, GLSL ES 3.00 for GL ES 3 (Android).
		// Other raylib profiles (GL 1.1 / 2.1, GL ES 2) have no dedicated
		// target and get sources unchanged.
		static inline cgss::ShaderPreprocessor::Target shader_preprocessor_target() {
			const int v = rlGetVersion();
			if (v == RL_OPENGL_33 || v == RL_OPENGL_43) return cgss::ShaderPreprocessor::Target::RaylibGlsl330;
			if (v == RL_OPENGL_ES_30) return cgss::ShaderPreprocessor::Target::RaylibGlslEs300;
			return cgss::ShaderPreprocessor::Target::DesktopFixedPipeline;
		}

		// raylib never pads textures to a power of two.
		static constexpr bool texture_supports_npot() noexcept { return true; }

		// Loads a program from optional vertex / fragment sources (nullptr =
		// raylib's default stage).
		static inline bool shader_load_program(native_shader& sh, const std::string* vsrc, const std::string* fsrc) {
			cgss::detail::ensureGLContext();
			if (!IsWindowReady()) return false;
			if (sh.id != 0) {
				raylib_detail::forgetShaderTextures(sh.id);
				UnloadShader(sh);
			}
			sh = LoadShaderFromMemory(vsrc ? vsrc->c_str() : nullptr, fsrc ? fsrc->c_str() : nullptr);
			// raylib falls back to its default shader on a compile/link error.
			return sh.id != 0 && sh.id != rlGetShaderIdDefault();
		}
		static inline bool shader_load_from_memory(native_shader& sh, const std::string& src, cgss::ShaderType type) {
			if (type == cgss::ShaderType::Fragment) return shader_load_program(sh, nullptr, &src);
			if (type == cgss::ShaderType::Vertex) return shader_load_program(sh, &src, nullptr);
			return false;
		}
		static inline bool shader_load_from_memory(native_shader& sh, const std::string& vsrc, const std::string& fsrc) {
			return shader_load_program(sh, &vsrc, &fsrc);
		}
		// 3-arg form takes a geometry shader, which raylib's default GL
		// profile doesn't support — refused outright.
		static inline bool shader_load_from_memory(native_shader&, const std::string&, const std::string&, const std::string&) {
			return false;
		}

		// Internal helper: resolves a uniform location once per call.
		// raylib's GetShaderLocation does a glGetUniformLocation under the
		// hood — cheap enough for per-frame uniform writes; cgss callers don't
		// expect us to cache.
		static inline int shader_loc(const native_shader& sh, const std::string& name) {
			cgss::detail::ensureGLContext();
			if (sh.id == 0) return -1;
			// The preprocessor renames some uniforms (the `texture` sampler...).
			return GetShaderLocation(sh, cgss::ShaderPreprocessor::uniformName(name, shader_preprocessor_target()).c_str());
		}

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, float v) {
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, &v, SHADER_UNIFORM_FLOAT);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, int v)   {
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, &v, SHADER_UNIFORM_INT);
		}
		// raylib has no SHADER_UNIFORM_BOOL — promote to int (0/1).
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, bool v)  {
			const int iv = v ? 1 : 0;
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, &iv, SHADER_UNIFORM_INT);
		}

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec2& v) {
			const float a[2] = { v.x, v.y };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_VEC2);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec3& v) {
			const float a[3] = { v.x, v.y, v.z };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_VEC3);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec4& v) {
			const float a[4] = { v.x, v.y, v.z, v.w };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_VEC4);
		}

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec2& v) {
			const int a[2] = { v.x, v.y };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC2);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec3& v) {
			const int a[3] = { v.x, v.y, v.z };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC3);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec4& v) {
			const int a[4] = { v.x, v.y, v.z, v.w };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC4);
		}

		// bvec*: GL has no boolean-vector uniform setter, the spec promotes
		// them to ints. Same path as ivec*.
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec2& v) {
			const int a[2] = { v.x ? 1 : 0, v.y ? 1 : 0 };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC2);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec3& v) {
			const int a[3] = { v.x ? 1 : 0, v.y ? 1 : 0, v.z ? 1 : 0 };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC3);
		}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec4& v) {
			const int a[4] = { v.x ? 1 : 0, v.y ? 1 : 0, v.z ? 1 : 0, v.w ? 1 : 0 };
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValue(sh, loc, a, SHADER_UNIFORM_IVEC4);
		}

		// Mat3: raylib's public API has no 3x3 setter (Matrix is 4x4-only).
		// Emit nothing for now — Mat3 uniforms are rare in fullscreen post-FX
		// shaders, the typical raylib use case here.
		static inline void shader_set_uniform(native_shader&, const std::string&, const cgss::Glsl::Mat3&) {}
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Mat4& m) {
			if (sh.id == 0) return;
			::Matrix M;
			// cgss Glsl::Mat4 stores row-major (m[row][col] ↔ array[row*4+col]).
			// raylib's Matrix is column-major (m0..m15 across rows of the column).
			// Transpose on load.
			M.m0  = m.array[0];  M.m4  = m.array[1];  M.m8  = m.array[2];  M.m12 = m.array[3];
			M.m1  = m.array[4];  M.m5  = m.array[5];  M.m9  = m.array[6];  M.m13 = m.array[7];
			M.m2  = m.array[8];  M.m6  = m.array[9];  M.m10 = m.array[10]; M.m14 = m.array[11];
			M.m3  = m.array[12]; M.m7  = m.array[13]; M.m11 = m.array[14]; M.m15 = m.array[15];
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValueMatrix(sh, loc, M);
		}

		// Kept per shader and bound when it draws (Internal/RaylibShaderTextures.h):
		// SetShaderValueTexture's slots are 4, and emptied by each batch flush.
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const native_texture& t) {
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			raylib_detail::setShaderTexture(sh.id, loc, t.id);
		}

		// CurrentTexture sentinel — the preprocessor renames the `texture`
		// sampler to `texture0`, which raylib auto-binds to the active
		// drawing texture during DrawTexturePro. Nothing to do here.
		static inline void shader_set_uniform_current_texture(native_shader&, const std::string&) {}

		static inline void shader_set_uniform_array(native_shader& sh, const std::string& name, const float* arr, std::size_t n) {
			if (n == 0) return;
			const int loc = shader_loc(sh, name); if (loc < 0) return;
			SetShaderValueV(sh, loc, arr, SHADER_UNIFORM_FLOAT, static_cast<int>(n));
		}

		// ---- Render-target view ops ----
		// target_set_view is the cross-cutting op the draw pipeline uses to
		// install a view (defview, viewport view, framed view, etc.) for the
		// upcoming draw calls. The raylib impl stashes the view in
		// raylib_detail::currentView() and — if the view has a sub-viewport —
		// opens a matching scissor. Callers restore the previous view by
		// invoking target_set_view again with the saved native_view value.
		//
		// Also stashes the target's pixel dimensions so the view-zoom and
		// scissor math sample the right "screen" — important inside
		// BeginTextureMode where the FBO's size differs from the window's
		// (e.g., scale!=1 windows or per-viewport-shader FBO compositing).
		static inline void target_set_view(native_render_target& t, const native_view& v) {
			cgss::detail::ensureGLContext();
			float tw, th;
			int fw, fh;
			if (t.texture.id != 0) {
				tw = static_cast<float>(t.texture.width);
				th = static_cast<float>(t.texture.height);
				fw = t.texture.width;
				fh = t.texture.height;
			} else {
				tw = static_cast<float>(IsWindowReady() ? GetScreenWidth()  : 0);
				th = static_cast<float>(IsWindowReady() ? GetScreenHeight() : 0);
				fw = IsWindowReady() ? GetRenderWidth()  : 0;
				fh = IsWindowReady() ? GetRenderHeight() : 0;
			}
			raylib_detail::setCurrentTargetSize(tw, th);
			raylib_detail::setCurrentFramebufferSize(fw, fh);
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
		// semantics: a world point (wx, wy) maps to target pixel
		//     pixel = viewport_px_center + (world - view_center) * zoom
		// where viewport_px_center = (vpX + vpW/2) * targetW,
		//       zoom              = (vpW * targetW) / sizeX.
		// targetW/H are the dimensions of the CURRENT draw target — the
		// window when drawing direct, or the FBO inside BeginTextureMode.
		// raylib's GetScreenWidth/Height always reports the window even
		// inside BeginTextureMode, so we read the target dims from
		// raylib_detail (written by target_set_view from the bound target).
		struct ViewDerived { float tx = 0.f, ty = 0.f, zoomX = 1.f, zoomY = 1.f, rotation = 0.f; };
		static inline ViewDerived derive_current_view() noexcept {
			const auto& v = raylib_detail::currentView();
			ViewDerived d;
			d.rotation = v.rotation;
			if (v.sizeX == 0.f || v.sizeY == 0.f) return d;
			float tw = raylib_detail::currentTargetWidth();
			float th = raylib_detail::currentTargetHeight();
			if (tw <= 0.f || th <= 0.f) {
				tw = static_cast<float>(IsWindowReady() ? GetScreenWidth()  : 0);
				th = static_cast<float>(IsWindowReady() ? GetScreenHeight() : 0);
			}
			d.zoomX = (v.vpW * tw) / v.sizeX;
			d.zoomY = (v.vpH * th) / v.sizeY;
			const float vpCenterX = v.vpX * tw + v.vpW * tw * 0.5f;
			const float vpCenterY = v.vpY * th + v.vpH * th * 0.5f;
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
			cgss::detail::ensureGLContext();
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

		// ---- Blending ----
		// cgss blend state is SFML's 6-field model (separate color / alpha
		// factors and equations). raylib's preset modes blend the alpha
		// channel like the color one (SRC_ALPHA, ONE_MINUS_SRC_ALPHA): drawn
		// into a viewport's render texture, a translucent sprite lowered its
		// alpha, darkening the viewport when composited. The state is applied
		// as-is through RL_BLEND_CUSTOM_SEPARATE (rlgl flushes the pending
		// batch whenever it actually changes).
		static inline int gl_blend_factor(cgss::BlendFactor f) noexcept {
			switch (f) {
				case cgss::BlendFactor::Zero:             return RL_ZERO;
				case cgss::BlendFactor::One:              return RL_ONE;
				case cgss::BlendFactor::SrcColor:         return RL_SRC_COLOR;
				case cgss::BlendFactor::OneMinusSrcColor: return RL_ONE_MINUS_SRC_COLOR;
				case cgss::BlendFactor::DstColor:         return RL_DST_COLOR;
				case cgss::BlendFactor::OneMinusDstColor: return RL_ONE_MINUS_DST_COLOR;
				case cgss::BlendFactor::SrcAlpha:         return RL_SRC_ALPHA;
				case cgss::BlendFactor::OneMinusSrcAlpha: return RL_ONE_MINUS_SRC_ALPHA;
				case cgss::BlendFactor::DstAlpha:         return RL_DST_ALPHA;
				case cgss::BlendFactor::OneMinusDstAlpha: return RL_ONE_MINUS_DST_ALPHA;
			}
			return RL_ONE;
		}
		static inline int gl_blend_equation(cgss::BlendEquation e) noexcept {
			switch (e) {
				case cgss::BlendEquation::Add:             return RL_FUNC_ADD;
				case cgss::BlendEquation::Subtract:        return RL_FUNC_SUBTRACT;
				case cgss::BlendEquation::ReverseSubtract: return RL_FUNC_REVERSE_SUBTRACT;
				case cgss::BlendEquation::Min:             return RL_MIN;
				case cgss::BlendEquation::Max:             return RL_MAX;
			}
			return RL_FUNC_ADD;
		}
		static inline void apply_blend(const native_render_states& rs) {
			cgss::detail::ensureGLContext();
			rlSetBlendFactorsSeparate(
				gl_blend_factor(rs.colorSrc), gl_blend_factor(rs.colorDst),
				gl_blend_factor(rs.alphaSrc), gl_blend_factor(rs.alphaDst),
				gl_blend_equation(rs.colorEq), gl_blend_equation(rs.alphaEq));
			rlSetBlendMode(RL_BLEND_CUSTOM_SEPARATE);
		}
		// Default state: SFML's BlendAlpha (the defaults of native_render_states).
		static inline void apply_default_blend() {
			static const native_render_states defaults{};
			apply_blend(defaults);
		}

		// ---- Draw thunks ----
		// Internal: applies the supplied render_states around a single draw
		// call — blend state, and the shader when one is compiled. Raylib's
		// shader / blend modes are global state, so they are set locally and
		// restored to the defaults afterwards (matching SFML's per-draw render
		// state semantics).
		//
		// A shader sees clip space as SFML gives it: spanning the view's
		// viewport, not the whole target. SFML sets glViewport to the view's
		// viewport; this backend places sprites itself in target pixels
		// (derive_current_view) under raylib's whole-target projection, and
		// clips with a scissor. Same pixels without a shader, but a vertex
		// shader working in clip space (PSDK's battle_shadow: its shadow
		// skewed by 0.2 of the viewport, of the window here) went too far. So
		// for a shader draw in a viewport smaller than its target, the GL
		// viewport and the projection cover that viewport only (the vertices
		// stay where they are), then raylib's are restored.
		template <class DrawFn>
		static inline void draw_with_render_states(const native_render_states* rs, DrawFn&& fn) {
			cgss::detail::ensureGLContext();
			const bool useShader = rs != nullptr && rs->shader != nullptr && rs->shader->id != 0;
			if (rs != nullptr) apply_blend(*rs);
			bool viewportSet = false;
			::Matrix projection{};
			if (useShader) {
				BeginShaderMode(*rs->shader);
				raylib_detail::bindShaderTextures(rs->shader->id);
				viewportSet = begin_shader_viewport(projection);
			}
			fn();
			if (useShader) {
				EndShaderMode();
				if (viewportSet) end_shader_viewport(projection);
			}
			if (rs != nullptr) apply_default_blend();
		}

		// The view's viewport as the GL viewport and projection of a shader
		// draw (draw_with_render_states). False when the viewport is the
		// whole target: nothing to change.
		// @param saved receives the projection to restore
		static inline bool begin_shader_viewport(::Matrix& saved) {
			const auto& v = raylib_detail::currentView();
			if (v.vpX <= 0.f && v.vpY <= 0.f && v.vpW >= 1.f && v.vpH >= 1.f) return false;
			const float tw = raylib_detail::currentTargetWidth();
			const float th = raylib_detail::currentTargetHeight();
			const int fw = raylib_detail::currentFramebufferWidth();
			const int fh = raylib_detail::currentFramebufferHeight();
			if (tw <= 0.f || th <= 0.f || fw <= 0 || fh <= 0) return false;
			// The viewport in target pixels (top-left origin, as the vertices)
			const float left = v.vpX * tw, top = v.vpY * th;
			const float right = left + v.vpW * tw, bottom = top + v.vpH * th;
			// In framebuffer pixels, from the bottom-left as GL counts: the
			// target's top row is the framebuffer's last, the window's or an
			// FBO's alike (raylib's projections put y = 0 at the top)
			const float sx = static_cast<float>(fw) / tw, sy = static_cast<float>(fh) / th;
			const int x = static_cast<int>(std::lround(left * sx));
			const int w = static_cast<int>(std::lround(right * sx)) - x;
			const int yTop = static_cast<int>(std::lround(top * sy));
			const int h = static_cast<int>(std::lround(bottom * sy)) - yTop;
			if (w <= 0 || h <= 0) return false;
			rlDrawRenderBatchActive();
			saved = rlGetMatrixProjection();
			rlViewport(x, fh - yTop - h, w, h);
			// raylib's projection maps the target's pixels to its framebuffer
			// (the modelview scales screen to render pixels on HiDPI): the
			// same projection, restricted to the viewport's rectangle
			rlMatrixMode(RL_PROJECTION);
			rlLoadIdentity();
			rlOrtho(left * sx, right * sx, bottom * sy, top * sy, 0.0, 1.0);
			rlMatrixMode(RL_MODELVIEW);
			return true;
		}

		static inline void end_shader_viewport(const ::Matrix& saved) {
			rlDrawRenderBatchActive();
			rlViewport(0, 0, raylib_detail::currentFramebufferWidth(), raylib_detail::currentFramebufferHeight());
			rlSetMatrixProjection(saved);
		}

		// Used by cgss::ViewportItem::draw to composite the off-screen
		// user-render-texture back onto the window with the bound shader
		// applied. Drives the entire viewport-level shader path.
		//
		// The sprite passed here is always the per-window userRenderSprite
		// whose backing texture is an FBO color attachment. OpenGL stores
		// FBO contents bottom-up (origin at lower-left) while raylib's
		// BeginTextureMode flips the projection so logical (0, 0) renders
		// to the top-left — net effect: the LOGICAL top row of content
		// ends up in texel row `texH - 1` of the underlying texture, and the
		// box's logical rows [boxTop, boxTop + boxH) are the GL rows
		// [texH - boxTop - boxH, texH - boxTop). DrawTexturePro samples a
		// negative-height source as the rows [y, y + |h|) upside down, so
		// source.y is the box's GL bottom row: `texH - boxTop - boxH`.
		// (`texH - boxTop` read the rows past the box: right only for a box
		// covering the whole FBO, through the texture's REPEAT wrap; a
		// letterboxed game area, boxTop = its origin, showed the wrong rows.)
		static inline void draw_sprite(native_render_target&, const native_sprite& s, const native_render_states& rs) {
			if (s.texture == nullptr) return;
			const int texH = static_cast<int>(s.texture->height);
			native_sprite flipped = s;
			flipped.textureRect.top    = texH - s.textureRect.top - s.textureRect.height;
			flipped.textureRect.height = -s.textureRect.height;
			draw_with_render_states(&rs, [&]() {
				draw_sprite_impl(flipped, flipped.x, flipped.y,
				                 flipped.originX, flipped.originY,
				                 flipped.scaleX, flipped.scaleY, 0.f);
			});
		}
		static inline void draw_sprite_with_transform(native_render_target&,
		                                              const native_sprite& s,
		                                              const native_transform& xform,
		                                              const native_render_states* base_states) {
			// `xform.angle` is already in screen-space (CW-positive) — see
			// build_transform above, which flips the cgss angle sign.
			draw_with_render_states(base_states, [&]() {
				draw_sprite_impl(s, xform.x, xform.y, xform.ox, xform.oy, xform.scaleX, xform.scaleY, xform.angle);
			});
		}
		static inline void draw_shape_with_transform(native_render_target&,
		                                             const cgss::ShapeGeometry& g,
		                                             const native_texture* tex,
		                                             const native_transform& xform,
		                                             const native_render_states* base_states) {
			draw_with_render_states(base_states, [&]() {
				draw_shape_untransformed(g, tex, xform.x, xform.y, xform.ox, xform.oy,
				                          xform.scaleX, xform.scaleY, xform.angle);
			});
		}

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
		// Unbinds the texture: draw_sprite_impl skips a sprite without one.
		static inline void clear_sprite_texture(native_sprite& s) noexcept {
			s.texture = nullptr;
			s.textureRect = cgss::IntRect{};
		}
		static inline void sprite_set_position(native_sprite& s, float x, float y) noexcept { s.x = x; s.y = y; }
		static inline void sprite_set_origin(native_sprite& s, float ox, float oy) noexcept { s.originX = ox; s.originY = oy; }
		static inline void sprite_set_scale(native_sprite& s, float sx, float sy) noexcept { s.scaleX = sx; s.scaleY = sy; }
		static inline void draw_sprite(native_render_target&, const native_sprite& s) {
			draw_sprite_impl(s, s.x, s.y, s.originX, s.originY, s.scaleX, s.scaleY, 0.f);
		}

		// Sprites sharing one texture, drawn back to back: rlgl merges
		// consecutive same-texture quads into one draw call by itself.
		static inline void draw_sprite_batch(native_render_target& target,
		                                     const native_sprite* const* sprites, std::size_t count) {
			cgss::detail::ensureGLContext();
			for (std::size_t i = 0; i < count; ++i) {
				draw_sprite(target, *sprites[i]);
			}
		}

		// Raylib draws against the currently-active BeginDrawing /
		// BeginTextureMode scope; the target is implicit. ScrollText emits
		// DrawTextPro via its own `draw()` method against that active state.
		static inline void draw_text(native_render_target&, const cgss::ScrollText& text) {
			cgss::detail::ensureGLContext();
			text.draw();
		}
		static inline Size2u get_sprite_bound_texture_size(const native_sprite& s) noexcept {
			if (s.texture == nullptr) return { 0u, 0u };
			return { static_cast<unsigned int>(s.texture->width),
			         static_cast<unsigned int>(s.texture->height) };
		}

		// ---- Shape ops ----
		// Rectangle / Circle support both fill and outline; Convex supports
		// fill (TriangleFan from the first vertex) and outline (LineStrip).
		// Textured shapes are NOT implemented — raylib's only textured-shape
		// primitive is DrawTexturePoly which requires pre-computed tex-coord
		// arrays that don't map onto ShapeGeometry cleanly. A textured
		// request falls through to solid-fill as a first-pass approximation.

		// Internal: apply the current view + an incoming transform to a
		// world-space point. Used by every shape-drawing path below.
		struct TransformedPoint { float x = 0.f, y = 0.f; };
		static inline TransformedPoint transform_point(float wx, float wy,
		                                               float posX, float posY,
		                                               float ox, float oy,
		                                               float scaleX, float scaleY,
		                                               float angleDegrees,
		                                               const ViewDerived& vd) noexcept {
			// Local -> world (origin-based + scale + rotation + translate).
			const float lx = (wx - ox) * scaleX;
			const float ly = (wy - oy) * scaleY;
			const float rad = (angleDegrees + vd.rotation) * 0.01745329252f; // deg -> rad
			const float c = std::cos(rad);
			const float s = std::sin(rad);
			const float rxWorld = lx * c - ly * s + posX;
			const float ryWorld = lx * s + ly * c + posY;
			// World -> screen via current view.
			return { vd.tx + rxWorld * vd.zoomX, vd.ty + ryWorld * vd.zoomY };
		}

		static inline void draw_shape_untransformed(const cgss::ShapeGeometry& g,
		                                            const native_texture* /*tex*/,
		                                            float posX, float posY,
		                                            float ox, float oy,
		                                            float scaleX, float scaleY,
		                                            float angleDegrees) {
			cgss::detail::ensureGLContext();
			const auto vd = derive_current_view();
			const ::Color fill{ g.fillColor.r, g.fillColor.g, g.fillColor.b, g.fillColor.a };
			const ::Color outline{ g.outlineColor.r, g.outlineColor.g, g.outlineColor.b, g.outlineColor.a };
			const float effScaleX = scaleX * vd.zoomX;
			const float effScaleY = scaleY * vd.zoomY;

			switch (g.type) {
			case cgss::ShapeType::Rectangle: {
				::Rectangle dst{
					vd.tx + posX * vd.zoomX,
					vd.ty + posY * vd.zoomY,
					g.width  * effScaleX,
					g.height * effScaleY };
				::Vector2 origin{ ox * effScaleX, oy * effScaleY };
				DrawRectanglePro(dst, origin, angleDegrees + vd.rotation, fill);
				if (g.outlineThickness > 0.f) {
					// Manual 4-line outline so rotation is honored (raylib's
					// DrawRectangleLinesEx is axis-aligned only). Corners in
					// shape-local space then transformed to screen.
					const auto p0 = transform_point(0.f,      0.f,      posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
					const auto p1 = transform_point(g.width,  0.f,      posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
					const auto p2 = transform_point(g.width,  g.height, posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
					const auto p3 = transform_point(0.f,      g.height, posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
					const float t = g.outlineThickness * vd.zoomX; // uniform-ish
					DrawLineEx(::Vector2{p0.x,p0.y}, ::Vector2{p1.x,p1.y}, t, outline);
					DrawLineEx(::Vector2{p1.x,p1.y}, ::Vector2{p2.x,p2.y}, t, outline);
					DrawLineEx(::Vector2{p2.x,p2.y}, ::Vector2{p3.x,p3.y}, t, outline);
					DrawLineEx(::Vector2{p3.x,p3.y}, ::Vector2{p0.x,p0.y}, t, outline);
				}
				break;
			}
			case cgss::ShapeType::Circle: {
				// Center of the cgss circle is (radius, radius) in local
				// coords — matches sf::CircleShape where the bounding box
				// origin is the top-left and getPoint(i) distributes around
				// the centered inscribed circle.
				const auto center = transform_point(g.radius, g.radius,
				                                    posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
				// raylib's DrawCircle is point-only — use CircleSector for
				// the fill so we can control segment count. Scale radius by
				// the effective zoom × scale. Use min(x,y) to stay circular
				// (no ellipse support — DrawEllipse exists but doesn't take
				// segments).
				const float effR = g.radius * (effScaleX < effScaleY ? effScaleX : effScaleY);
				const int   segs = g.pointCount > 3 ? static_cast<int>(g.pointCount) : 3;
				DrawCircleSector(::Vector2{center.x, center.y}, effR, 0.f, 360.f, segs, fill);
				if (g.outlineThickness > 0.f) {
					const float t = g.outlineThickness * vd.zoomX;
					const float inner = effR > t ? effR - t * 0.5f : effR * 0.5f;
					const float outer = effR + t * 0.5f;
					DrawRing(::Vector2{center.x, center.y}, inner, outer, 0.f, 360.f, segs, outline);
				}
				break;
			}
			case cgss::ShapeType::Convex: {
				if (g.points.size() < 3) break;
				// Transform each local point into screen coords, then build a
				// triangle fan for fill + line strip for outline. The fan is
				// rooted at points[0] so the polygon must be convex (matches
				// sf::ConvexShape's constraint).
				std::vector<::Vector2> screen;
				screen.reserve(g.points.size() + 1);
				for (const auto& p : g.points) {
					const auto tp = transform_point(p.x, p.y, posX, posY, ox, oy, scaleX, scaleY, angleDegrees, vd);
					screen.push_back(::Vector2{tp.x, tp.y});
				}
				// DrawTriangleFan signature: (points, pointCount, color).
				// Iterates in CCW order; if our input is CW we'd need to
				// reverse, but cgss/SFML convex-shape convention is CCW.
				DrawTriangleFan(screen.data(), static_cast<int>(screen.size()), fill);
				if (g.outlineThickness > 0.f) {
					screen.push_back(screen.front()); // close the loop
					DrawLineStrip(screen.data(), static_cast<int>(screen.size()), outline);
				}
				break;
			}
			}
		}

		static inline void draw_shape(native_render_target&, const cgss::ShapeGeometry& g,
		                              const native_texture* tex, const native_render_states&) {
			// Without a transform, render states are the sprite's own
			// (all zeros in current cgss::Shape flow); fall through to the
			// identity transform.
			draw_shape_untransformed(g, tex, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f);
		}

		// get_shape_point: analytical per-type since no native shape is
		// retained. Matches sf::Shape::getPoint semantics (local coords).
		static inline cgss::Point get_shape_point(const cgss::ShapeGeometry& g, std::size_t index) {
			switch (g.type) {
			case cgss::ShapeType::Rectangle: {
				switch (index % 4) {
				case 0: return cgss::Point{ 0.f,     0.f };
				case 1: return cgss::Point{ g.width, 0.f };
				case 2: return cgss::Point{ g.width, g.height };
				default: return cgss::Point{ 0.f,    g.height };
				}
			}
			case cgss::ShapeType::Circle: {
				const float pc = static_cast<float>(g.pointCount == 0 ? 1 : g.pointCount);
				const float theta = (static_cast<float>(index) / pc) * 6.2831853071795864f;
				return cgss::Point{
					g.radius + std::cos(theta) * g.radius,
					g.radius + std::sin(theta) * g.radius };
			}
			case cgss::ShapeType::Convex:
				if (index < g.points.size()) return g.points[index];
				return {};
			}
			return {};
		}

		// get_shape_global_bounds: axis-aligned bounding box of the shape
		// after the transform is applied. Matches sf::Shape::getGlobalBounds
		// for the non-rotated case; for rotated shapes we return the axis-
		// aligned bound of the rotated box rather than the unrotated box
		// (SFML semantics). Good enough for getWidth()/getHeight() callers.
		struct Bounds2f { float width = 0.f; float height = 0.f; };
		static inline Bounds2f get_shape_global_bounds(const cgss::ShapeGeometry& g, const cgss::Transformable& xf) {
			float localW = 0.f, localH = 0.f;
			switch (g.type) {
			case cgss::ShapeType::Rectangle: localW = g.width;       localH = g.height;       break;
			case cgss::ShapeType::Circle:    localW = g.radius * 2;  localH = g.radius * 2;   break;
			case cgss::ShapeType::Convex: {
				// min/max sweep over convex vertices.
				if (g.points.empty()) break;
				float minX = g.points[0].x, maxX = minX;
				float minY = g.points[0].y, maxY = minY;
				for (const auto& p : g.points) {
					if (p.x < minX) minX = p.x; else if (p.x > maxX) maxX = p.x;
					if (p.y < minY) minY = p.y; else if (p.y > maxY) maxY = p.y;
				}
				localW = maxX - minX;
				localH = maxY - minY;
				break;
			}
			}
			const float rad = xf.getAngle() * 0.01745329252f;
			const float c = std::cos(rad), s = std::sin(rad);
			const float sw = localW * xf.getScaleX();
			const float sh = localH * xf.getScaleY();
			// AABB of the rotated rect (c + s always positive absolute).
			const float absc = c < 0 ? -c : c;
			const float abss = s < 0 ? -s : s;
			return Bounds2f{ sw * absc + sh * abss, sw * abss + sh * absc };
		}

		// ---- Texture ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			cgss::detail::ensureGLContext();
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
			cgss::detail::ensureGLContext();
			UpdateTexture(t, i.data);
		}

		// Create a GPU-backed texture from a CPU-side image. Releases any
		// existing GPU backing first. Used by ext/Sprite to move bitmap
		// data onto the GPU for drawing.
		// false when the GPU refuses the texture (larger than its maximum size)
		// Refuses an image beyond GL_MAX_TEXTURE_SIZE like SFML does: drivers
		// differ on what they do with it (error, garbage, or accepting it).
		static inline bool texture_create_from_image(native_texture& t, const native_image& i) {
			cgss::detail::ensureGLContext();
			if (t.id != 0) UnloadTexture(t);
			t = native_texture{};
			// Only against the driver's own limit: the fallback guess would
			// refuse textures the GPU takes.
			const auto maxSize = static_cast<int>(raylib_detail::queriedMaxTextureSize());
			if (maxSize > 0 && (i.width > maxSize || i.height > maxSize)) {
				TraceLog(LOG_WARNING, "TEXTURE: %dx%d exceeds the GPU maximum %dx%d", i.width, i.height, maxSize, maxSize);
				return false;
			}
			t = LoadTextureFromImage(i);
			return t.id != 0;
		}

		// Deep-release a texture's GPU backing. Safe on zero-initialized
		// textures (id == 0).
		static inline void texture_destroy(native_texture& t) noexcept {
			cgss::detail::ensureGLContext();
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
			cgss::detail::ensureGLContext();
			BeginScissorMode(clipX, clipY, clipW, clipH);
			rlPushMatrix();
			rlTranslatef(offsetX, offsetY, 0.f);
			rlScalef(scaleX, scaleY, 1.f);
		}
		static inline void viewport_end(native_render_target& /*target*/) {
			cgss::detail::ensureGLContext();
			rlPopMatrix();
			EndScissorMode();
		}

		// Filled rect fill (used by ext/Viewport's begin_draw to show the
		// viewport area). Immediate-mode — draws directly to the current
		// BeginDrawing context.
		static inline void draw_filled_rect(native_render_target& /*target*/,
		                                    int x, int y, int w, int h, cgss::Color c) {
			cgss::detail::ensureGLContext();
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
			cgss::detail::ensureGLContext();
			// In the target's current view, like SFML's draw (and draw_sprite):
			// the destination is in view coordinates, not target pixels. A
			// window scaled 2x drew the framed views' skins at half size.
			const auto vd = derive_current_view();
			::Rectangle src{
				static_cast<float>(srcRect.left),  static_cast<float>(srcRect.top),
				static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			::Rectangle dst{
				vd.tx + dstX * vd.zoomX, vd.ty + dstY * vd.zoomY,
				dstWidth * vd.zoomX, dstHeight * vd.zoomY };
			::Vector2 origin{ originX * vd.zoomX, originY * vd.zoomY };
			DrawTexturePro(tex, src, dst, origin, angleDegrees + vd.rotation,
			               ::Color{ tint.r, tint.g, tint.b, tint.a });
		}

		// Update a sub-rectangle of a texture from a raw pixel buffer. The
		// source buffer layout must be R8G8B8A8 at the given stride; raylib
		// handles the upload via UpdateTextureRec. Size then position, as
		// SFML's (sf::Texture::update): it took the position first, and
		// YukiGif's frames (w x h at 0, 0) were written 0x0 at (w, h).
		static inline void texture_update_pixels(native_texture& t, const std::uint8_t* pixels,
		                                        unsigned int w, unsigned int h,
		                                        unsigned int x, unsigned int y) {
			cgss::detail::ensureGLContext();
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
			::Rectangle sr{ static_cast<float>(srcRect.left),  static_cast<float>(srcRect.top),
			                static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			::Rectangle dr{ static_cast<float>(x),             static_cast<float>(y),
			                static_cast<float>(srcRect.width), static_cast<float>(srcRect.height) };
			if (applyAlpha) {
				// Alpha-blend src over dst (raylib's default ImageDraw).
				// Fully-qualified ::Color{} to avoid the raylib WHITE macro
				// colliding with `cgss::Color` via namespace lookup.
				ImageDraw(&dst, src, sr, dr, ::Color{ 255, 255, 255, 255 });
				return;
			}
			// applyAlpha=false → REPLACE: copy source pixels (including their
			// own alpha) into dst, matching sf::Image::copy(applyAlpha=false).
			// raylib's ImageDraw always alpha-blends, so do a manual row-wise
			// memcpy after clipping. Both images are assumed to be
			// PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 (the only format the litecgss
			// Image pipeline produces — see image_create_with_pixels and
			// basic_ImageEmptySerializeMethod). If a caller hands us a
			// different format, fall back to alpha-blend rather than
			// corrupting pixels.
			constexpr int kRGBA8 = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			if (src.format != kRGBA8 || dst.format != kRGBA8 ||
			    src.data == nullptr || dst.data == nullptr) {
				ImageDraw(&dst, src, sr, dr, ::Color{ 255, 255, 255, 255 });
				return;
			}
			// Clip src rect into source bounds.
			int sx0 = srcRect.left;
			int sy0 = srcRect.top;
			int sx1 = sx0 + srcRect.width;
			int sy1 = sy0 + srcRect.height;
			int dx0 = static_cast<int>(x);
			int dy0 = static_cast<int>(y);
			if (sx0 < 0) { dx0 += -sx0; sx0 = 0; }
			if (sy0 < 0) { dy0 += -sy0; sy0 = 0; }
			if (sx1 > src.width)  sx1 = src.width;
			if (sy1 > src.height) sy1 = src.height;
			if (sx0 >= sx1 || sy0 >= sy1) return;
			int dx1 = dx0 + (sx1 - sx0);
			int dy1 = dy0 + (sy1 - sy0);
			if (dx0 < 0) { sx0 += -dx0; dx0 = 0; }
			if (dy0 < 0) { sy0 += -dy0; dy0 = 0; }
			if (dx1 > dst.width)  dx1 = dst.width;
			if (dy1 > dst.height) dy1 = dst.height;
			if (dx0 >= dx1 || dy0 >= dy1) return;
			const int copyW = dx1 - dx0;
			const int copyH = dy1 - dy0;
			auto*       dstBytes = static_cast<std::uint8_t*>(dst.data);
			const auto* srcBytes = static_cast<const std::uint8_t*>(src.data);
			const std::size_t rowBytes = static_cast<std::size_t>(copyW) * 4u;
			for (int row = 0; row < copyH; ++row) {
				const std::size_t srcOff = (static_cast<std::size_t>(sy0 + row) * src.width + sx0) * 4u;
				const std::size_t dstOff = (static_cast<std::size_t>(dy0 + row) * dst.width + dx0) * 4u;
				std::memcpy(dstBytes + dstOff, srcBytes + srcOff, rowBytes);
			}
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

		// PNG codec layer for the unified Image/Texture serializers.
		// raylib's stb-image-based decoder autodetects the format from the
		// buffer header regardless of the extension hint, so ".png" is fine,
		// among the decoders raylib was built with: its packagers turn on
		// SFML's set (JPG, TGA, PSD, HDR, PIC; tools/deps/build-deps.sh).
		// Empty-image creation goes via GenImageColor with a transparent fill.
		static inline bool png_decode_memory(native_image& image, const void* data, std::size_t size) {
			image_destroy(image);
			image = LoadImageFromMemory(".png",
			                            reinterpret_cast<const unsigned char*>(data),
			                            static_cast<int>(size));
			return image.data != nullptr;
		}

		static inline bool png_decode_file(native_image& image, const char* filename) {
			image_destroy(image);
			image = LoadImage(filename);
			return image.data != nullptr;
		}

		// raylib's ExportImageToMemory allocates with RL_MALLOC, which
		// defaults to plain malloc — so std::free in the standard saver
		// teardown is compatible. Document as a config-coupled assumption
		// (custom allocator builds would need to swap to MemFree on the
		// caller side).
		static inline cgss::MemorySerializerData png_encode_memory(const native_image& image) {
			if (image.data == nullptr) return { nullptr, 0u };
			int size = 0;
			unsigned char* bytes = ExportImageToMemory(image, ".png", &size);
			if (bytes == nullptr || size <= 0) return { nullptr, 0u };
			return { bytes, static_cast<std::size_t>(size) };
		}

		// Returns 0 on success — matches the SFML/lodepng convention.
		static inline unsigned int png_encode_file(const native_image& image, const char* filename) {
			if (image.data == nullptr) return 1u;
			return ExportImage(image, filename) ? 0u : 1u;
		}

		static inline bool image_create_blank_rgba(native_image& image, unsigned int width, unsigned int height) {
			image_destroy(image);
			image = GenImageColor(static_cast<int>(width), static_cast<int>(height), ::Color{ 0, 0, 0, 0 });
			return image.data != nullptr;
		}

		// Read GPU texels back to a CPU image (used by the texture saver).
		// raylib has no in-place equivalent — we LoadImageFromTexture, then
		// the caller owns the freshly-allocated image and must release it
		// (typically via image_destroy when the cgss::Image wrapper goes out
		// of scope).
		static inline bool texture_read_to_image(const native_texture& t, native_image& image) {
			cgss::detail::ensureGLContext();
			if (t.id == 0) return false;
			image_destroy(image);
			image = LoadImageFromTexture(t);
			return image.data != nullptr;
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
