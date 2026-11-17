#ifndef CGSS_BACKEND_SFML_BACKEND_H
#define CGSS_BACKEND_SFML_BACKEND_H

// Full SFML 2.6 reference backend: includes SFMLBackendFwd.h (typedefs +
// capability tags) and defines `SFMLBackend::Ops` with all inline static
// thunks. Header-only by design — every operation is `static inline` so
// release builds inline the wrapper down to the direct SFML call.
//
// Include this only from implementation .cpp files. Public templates use
// SFMLBackendFwd.h to avoid pulling SFML into consumer TUs.

#include "LiteCGSS/Backend/SFMLBackendFwd.h"

#include <cstdint>
#include <vector>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Sensor.hpp>
#include <SFML/System/String.hpp>

#include <cstdlib>
#include <optional>
#include <string>

#include <lodepng.h>

#include "Backend/Internal/SFMLCapabilities.h"
#include "Backend/Internal/SFMLConversions.h"
#include "Backend/Shader/ShaderPreprocessor.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Configuration/VideoMode.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Common/Vector3.h"
#include "LiteCGSS/Events/EventPayloads.h"
#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Events/SensorType.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Transformable.h"
#include "Backend/Specializations/sfml/ScrollText.h"

namespace cgss::backend {

	// Definition of the nested Ops struct forward-declared in SFMLBackendFwd.h.
	// Each method is a thin `static inline` thunk over an SFML call so the
	// compiler can inline it through the wrapper template at the use site.
	struct SFMLBackend::Ops {

		// Conversion helpers (toNative / fromNative overloads for Color,
		// IntRect, BlendFactor/Equation, ShaderType, Glsl values, ContextSettings)
		// live in Backend/Internal/SFMLConversions.h as free functions in
		// cgss::backend::. Ops-body callers use them via unqualified lookup —
		// member function name lookup falls through to the enclosing
		// cgss::backend namespace, and ADL on cgss:: or sf:: arguments also
		// reaches them.

		// ---- Wrapper native accessors ----
		// Backend-side replacement for the (now-private) `wrapper.raw()`
		// escape hatch. Each cgss wrapper friends this Ops struct so impl-
		// side .cpp files can reach the underlying native handle while
		// consumers cannot. Use the free `cgss::backend::native_of` helper
		// below at call sites — it forwards through here.
		template <class W>
		static inline auto& native_of(W& w) noexcept {
			return w.native();
		}
		template <class W>
		static inline const auto& native_of(const W& w) noexcept {
			return w.native();
		}

		// Shared-ownership handle on a Texture's native backing. Replaces
		// the public `basic_Texture::ref()` escape hatch (now removed).
		// Used by Sprite/Shape setTexture to keep the native alive across
		// the lifetime of the drawable regardless of whether the source
		// cgss::Texture is still in scope. `Ops` is friend of basic_Texture,
		// so this reaches the private m_texture member directly.
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
		static inline sf::Transform build_transform(const cgss::Transformable& xf) noexcept {
			sf::Transform t;
			t.translate(xf.getX(), xf.getY());
			t.rotate(-xf.getAngle());
			t.scale(xf.getScaleX(), xf.getScaleY());
			t.translate(-xf.getOx(), -xf.getOy());
			return t;
		}

		// ---- Window ops (simple state) ----
		// Complex window operations (create, set_icon, poll/wait_event, render
		// texture choreography) are NOT abstracted yet — DisplayWindow's body
		// retains direct sf:: calls for those, deferred until either full
		// `basic_DisplayWindow<RaylibBackend>` specialization or a follow-up
		// op-coverage round.
		static inline void window_close(native_window& w)               { w.close(); }
		static inline bool window_is_open(const native_window& w)       { return w.isOpen(); }
		static inline void window_clear(native_window& w)               { w.clear(); }
		static inline void window_display(native_window& w)             { w.display(); }
		static inline void window_set_active(native_window& w, bool a)  { w.setActive(a); }
		static inline void window_set_title(native_window& w, const char* t) { w.setTitle(t); }
		static inline void window_set_size(native_window& w, unsigned int width, unsigned int height) {
			w.setSize(sf::Vector2u{ width, height });
		}
		static inline Size2u window_get_size(const native_window& w) noexcept {
			const auto s = w.getSize();
			return { s.x, s.y };
		}
		static inline void window_set_mouse_cursor_visible(native_window& w, bool v) { w.setMouseCursorVisible(v); }
		static inline void window_set_vsync(native_window& w, bool v)            { w.setVerticalSyncEnabled(v); }
		static inline void window_set_framerate_limit(native_window& w, unsigned int f) { w.setFramerateLimit(f); }
		static inline void window_set_position(native_window& w, int x, int y) {
			w.setPosition(sf::Vector2i{ x, y });
		}
		static inline Point2i window_get_position(const native_window& w) noexcept {
			const auto p = w.getPosition();
			return { p.x, p.y };
		}
		static inline native_view window_get_default_view(const native_window& w) {
			return w.getDefaultView();
		}
		static inline void window_set_view(native_window& w, const native_view& v) {
			w.setView(v);
		}
		static inline cgss::DisplayWindowContextSettings window_get_settings(const native_window& w) {
			const auto& c = w.getSettings();
			return cgss::DisplayWindowContextSettings{
				c.depthBits, c.stencilBits, c.antialiasingLevel,
				c.majorVersion, c.minorVersion, c.attributeFlags, c.sRgbCapable
			};
		}
		static inline bool window_poll_event(native_window& w, native_event& ev) {
			return w.pollEvent(ev);
		}
		static inline bool window_wait_event(native_window& w, native_event& ev) {
			return w.waitEvent(ev);
		}

		// ---- Event translation ops ----
		// Translate an already-polled `sf::Event` to cgss-typed payloads.
		// Enum layout contract with SFML 2.6 — these cgss enums have the same
		// numeric values as their sf:: counterparts, so a `static_cast` is the
		// whole conversion (keep them in sync if either side changes):
		//   cgss::EventType   == sf::Event::EventType (7, the deprecated
		//                        MouseWheelMoved, is folded into 8 below)
		//   cgss::KeyCode     == sf::Keyboard::Key for 0..100
		//   cgss::MouseButton == sf::Mouse::Button (Left..XButton2)
		//   cgss::SensorType  == sf::Sensor::Type
		//   cgss::BlendFactor / BlendEquation / ShaderType: see SFMLConversions.h
		// cgss event payload field names mirror sf::Event's sub-structs.
		// Gamepad button/axis and the deprecated-wheel event need real
		// re-mapping — handled by the small `to_cgss_*` helpers defined at the
		// bottom of this struct.

		static inline cgss::EventType event_type(const native_event& e) noexcept {
			// SFML's MouseWheelMoved (type 7) is deprecated but still emitted;
			// collapse it to MouseWheelScrolled for consumers.
			if (e.type == sf::Event::MouseWheelMoved) {
				return cgss::EventType::MouseWheelScrolled;
			}
			return static_cast<cgss::EventType>(e.type);
		}

		// A synthetic key event (cgss::input::injectKey). cgss::KeyCode and
		// cgss::Scancode share SFML's numbering (see event_as_key_press).
		static inline void event_set_key(native_event& e, bool pressed, const cgss::KeyEvent& key) {
			e = native_event{};
			e.type = pressed ? sf::Event::KeyPressed : sf::Event::KeyReleased;
			e.key.code = static_cast<sf::Keyboard::Key>(key.code);
			e.key.scancode = static_cast<sf::Keyboard::Scancode>(key.scancode);
			e.key.alt = key.alt;
			e.key.control = key.control;
			e.key.shift = key.shift;
			e.key.system = key.system;
		}

		static inline std::optional<cgss::KeyEvent> event_as_key_press(const native_event& e) {
			if (e.type != sf::Event::KeyPressed) return std::nullopt;
			return cgss::KeyEvent{
				static_cast<cgss::KeyCode>(e.key.code),
				e.key.alt, e.key.control, e.key.shift, e.key.system,
				static_cast<cgss::Scancode>(e.key.scancode)
			};
		}
		static inline std::optional<cgss::KeyEvent> event_as_key_release(const native_event& e) {
			if (e.type != sf::Event::KeyReleased) return std::nullopt;
			return cgss::KeyEvent{
				static_cast<cgss::KeyCode>(e.key.code),
				e.key.alt, e.key.control, e.key.shift, e.key.system,
				static_cast<cgss::Scancode>(e.key.scancode)
			};
		}
		static inline std::optional<cgss::TextEvent> event_as_text_entered(const native_event& e) {
			if (e.type != sf::Event::TextEntered) return std::nullopt;
			return cgss::TextEvent{ e.text.unicode };
		}
		static inline std::optional<cgss::SizeEvent> event_as_resize(const native_event& e) {
			if (e.type != sf::Event::Resized) return std::nullopt;
			return cgss::SizeEvent{ e.size.width, e.size.height };
		}
		static inline std::optional<cgss::MouseMoveEvent> event_as_mouse_move(const native_event& e) {
			if (e.type != sf::Event::MouseMoved) return std::nullopt;
			return cgss::MouseMoveEvent{ e.mouseMove.x, e.mouseMove.y };
		}
		static inline std::optional<cgss::MouseButtonEvent> event_as_mouse_button_press(const native_event& e) {
			if (e.type != sf::Event::MouseButtonPressed) return std::nullopt;
			return cgss::MouseButtonEvent{
				static_cast<cgss::MouseButton>(e.mouseButton.button),
				e.mouseButton.x, e.mouseButton.y
			};
		}
		static inline std::optional<cgss::MouseButtonEvent> event_as_mouse_button_release(const native_event& e) {
			if (e.type != sf::Event::MouseButtonReleased) return std::nullopt;
			return cgss::MouseButtonEvent{
				static_cast<cgss::MouseButton>(e.mouseButton.button),
				e.mouseButton.x, e.mouseButton.y
			};
		}
		static inline std::optional<cgss::MouseWheelEvent> event_as_mouse_wheel(const native_event& e) {
			if (e.type == sf::Event::MouseWheelScrolled) {
				return cgss::MouseWheelEvent{
					e.mouseWheelScroll.delta,
					e.mouseWheelScroll.x, e.mouseWheelScroll.y,
					e.mouseWheelScroll.wheel == sf::Mouse::HorizontalWheel
				};
			}
			if (e.type == sf::Event::MouseWheelMoved) {
				// Deprecated SFML event; carries no wheel-axis info. Assume
				// vertical (the only wheel this event ever reported).
				return cgss::MouseWheelEvent{
					static_cast<float>(e.mouseWheel.delta),
					e.mouseWheel.x, e.mouseWheel.y, false
				};
			}
			return std::nullopt;
		}
		static inline std::optional<cgss::GamepadButtonEvent> event_as_gamepad_button_press(const native_event& e) {
			if (e.type != sf::Event::JoystickButtonPressed) return std::nullopt;
			return cgss::GamepadButtonEvent{
				e.joystickButton.joystickId,
				from_sfml_joystick_button(e.joystickButton.button)
			};
		}
		static inline std::optional<cgss::GamepadButtonEvent> event_as_gamepad_button_release(const native_event& e) {
			if (e.type != sf::Event::JoystickButtonReleased) return std::nullopt;
			return cgss::GamepadButtonEvent{
				e.joystickButton.joystickId,
				from_sfml_joystick_button(e.joystickButton.button)
			};
		}
		static inline std::optional<cgss::GamepadMoveEvent> event_as_gamepad_move(const native_event& e) {
			if (e.type != sf::Event::JoystickMoved) return std::nullopt;
			return cgss::GamepadMoveEvent{
				e.joystickMove.joystickId,
				from_sfml_joystick_axis(e.joystickMove.axis),
				e.joystickMove.position / 100.f // SFML [-100..100] -> cgss [-1..1]
			};
		}
		static inline std::optional<cgss::GamepadConnectEvent> event_as_gamepad_connect(const native_event& e) {
			if (e.type != sf::Event::JoystickConnected) return std::nullopt;
			return cgss::GamepadConnectEvent{ e.joystickConnect.joystickId };
		}
		static inline std::optional<cgss::GamepadConnectEvent> event_as_gamepad_disconnect(const native_event& e) {
			if (e.type != sf::Event::JoystickDisconnected) return std::nullopt;
			return cgss::GamepadConnectEvent{ e.joystickConnect.joystickId };
		}
		static inline std::optional<cgss::TouchEvent> event_as_touch(const native_event& e) {
			cgss::TouchPhase phase;
			switch (e.type) {
				case sf::Event::TouchBegan: phase = cgss::TouchPhase::Began; break;
				case sf::Event::TouchMoved: phase = cgss::TouchPhase::Moved; break;
				case sf::Event::TouchEnded: phase = cgss::TouchPhase::Ended; break;
				default: return std::nullopt;
			}
			return cgss::TouchEvent{ e.touch.finger, e.touch.x, e.touch.y, phase };
		}
		static inline std::optional<cgss::SensorEvent> event_as_sensor(const native_event& e) {
			if (e.type != sf::Event::SensorChanged) return std::nullopt;
			return cgss::SensorEvent{
				static_cast<cgss::SensorType>(e.sensor.type),
				e.sensor.x, e.sensor.y, e.sensor.z
			};
		}

		// ---- Input ops (synchronous state queries) ----

		// Keyboard — cgss::KeyCode values 0..100 match sf::Keyboard::Key. cgss
		// extensions (>=101: CapsLock, NumLock, ...) are not in SFML 2.6's
		// base enum, so they return false here. A future upgrade can wire
		// them via sf::Keyboard::Scan.
		static inline bool keyboard_is_key_pressed(cgss::KeyCode code) {
			const auto i = static_cast<int>(code);
			if (i < 0 || i >= static_cast<int>(sf::Keyboard::KeyCount)) return false;
			return sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(i));
		}		// cgss::Scancode mirrors sf::Keyboard::Scancode (SFML 2.6) 1:1.
		static inline bool keyboard_is_scancode_pressed(cgss::Scancode scancode) {
			const int i = static_cast<int>(scancode);
			if (i < 0 || i >= static_cast<int>(sf::Keyboard::Scan::ScancodeCount)) return false;
			return sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Scancode>(i));
		}
		static inline cgss::KeyCode keyboard_localize(cgss::Scancode scancode) {
			const int i = static_cast<int>(scancode);
			if (i < 0 || i >= static_cast<int>(sf::Keyboard::Scan::ScancodeCount)) return cgss::KeyCode::Unknown;
			return static_cast<cgss::KeyCode>(sf::Keyboard::localize(static_cast<sf::Keyboard::Scancode>(i)));
		}
		static inline cgss::Scancode keyboard_delocalize(cgss::KeyCode key) {
			const int i = static_cast<int>(key);
			if (i < 0 || i >= static_cast<int>(sf::Keyboard::KeyCount)) return cgss::Scancode::Unknown;
			return static_cast<cgss::Scancode>(sf::Keyboard::delocalize(static_cast<sf::Keyboard::Key>(i)));
		}


		// Mouse
		static inline bool mouse_is_button_pressed(cgss::MouseButton btn) {
			const auto i = static_cast<int>(btn);
			// VerticalWheel/HorizontalWheel are event-only under SFML.
			if (i < 0 || i >= static_cast<int>(sf::Mouse::ButtonCount)) return false;
			return sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(i));
		}
		static inline Point2i mouse_get_position() {
			const auto p = sf::Mouse::getPosition();
			return { p.x, p.y };
		}
		static inline Point2i mouse_get_position(const native_window& w) {
			const auto p = sf::Mouse::getPosition(w);
			return { p.x, p.y };
		}
		static inline void mouse_set_position(int x, int y) {
			sf::Mouse::setPosition(sf::Vector2i{ x, y });
		}
		static inline void mouse_set_position(int x, int y, const native_window& w) {
			sf::Mouse::setPosition(sf::Vector2i{ x, y }, w);
		}
		// SFML 2.6 exposes wheel delta only via events — no state query.
		// Callers must gate on `has_wheel_state_query` before calling.
		static inline float mouse_wheel_delta() noexcept { return 0.f; }

		// Gamepad — sf::Joystick uses opaque 0..31 button indices, so
		// `gamepad_is_button_pressed(GamepadButton)` needs a cgss->index
		// mapping via `to_sfml_joystick_button`. The mapping follows XInput
		// on Linux (SDL GameController layout) and may misalign on other
		// drivers; callers that need exact button indices can use
		// `gamepad_is_raw_button_pressed(id, idx)` as an escape hatch in a
		// future revision.
		static inline bool gamepad_is_connected(unsigned int id) {
			return sf::Joystick::isConnected(id);
		}
		static inline bool gamepad_is_button_pressed(unsigned int id, cgss::GamepadButton btn) {
			const auto idx = to_sfml_joystick_button(btn);
			if (idx < 0) return false;
			return sf::Joystick::isButtonPressed(id, static_cast<unsigned int>(idx));
		}
		static inline bool gamepad_is_button_released(unsigned int id, cgss::GamepadButton btn) {
			return !gamepad_is_button_pressed(id, btn);
		}
		static inline float gamepad_axis_position(unsigned int id, cgss::GamepadAxis axis) {
			return sf::Joystick::getAxisPosition(id, to_sfml_joystick_axis(axis)) / 100.f;
		}
		static inline unsigned int gamepad_axis_count(unsigned int id) {
			// SFML has no "axis count" — approximate with button count or
			// hardcode AxisCount. Return 0 for disconnected pads.
			if (!sf::Joystick::isConnected(id)) return 0u;
			return static_cast<unsigned int>(sf::Joystick::AxisCount);
		}
		static inline const char* gamepad_name(unsigned int id) {
			// sf::String -> stable C string. Return empty string on
			// disconnected. Note: the returned pointer is backed by a
			// function-local static, so it is overwritten by subsequent calls.
			static std::string name;
			if (!sf::Joystick::isConnected(id)) { name.clear(); return name.c_str(); }
			name = sf::Joystick::getIdentification(id).name.toAnsiString();
			return name.c_str();
		}
		static inline void gamepad_set_vibration(unsigned int /*id*/, float /*left*/, float /*right*/, float /*ms*/) {
			// SFML 2.6 has no rumble / vibration API. No-op; loggers can
			// observe the first call via a scratch flag (out of scope here).
		}
		static inline void gamepad_update() {
			sf::Joystick::update();
		}

		// Touch — SFML exposes touch only through events, not sync state.
		// `has_input_touch = false` on this backend; these are present for
		// Ops-symmetry with raylib.
		static inline unsigned int touch_count() noexcept { return 0u; }
		static inline Point2i      touch_position(unsigned int /*index*/) noexcept { return {}; }
		static inline unsigned int touch_id(unsigned int /*index*/) noexcept { return 0u; }

		// ---- Clipboard ops ----
		static inline std::string clipboard_get_string() {
			const auto bytes = sf::Clipboard::getString().toUtf8();
			return std::string(bytes.begin(), bytes.end());
		}
		static inline void clipboard_set_string(const std::string& utf8) {
			sf::Clipboard::setString(sf::String::fromUtf8(utf8.begin(), utf8.end()));
		}

		// ---- Sensor ops ----
		// cgss::SensorType mirrors sf::Sensor::Type 1:1.
		static inline bool sensor_in_range(cgss::SensorType type) noexcept {
			const int i = static_cast<int>(type);
			return i >= 0 && i < static_cast<int>(sf::Sensor::Count);
		}
		static inline bool sensor_is_available(cgss::SensorType type) {
			return sensor_in_range(type) && sf::Sensor::isAvailable(static_cast<sf::Sensor::Type>(type));
		}
		static inline void sensor_set_enabled(cgss::SensorType type, bool enabled) {
			if (sensor_in_range(type)) sf::Sensor::setEnabled(static_cast<sf::Sensor::Type>(type), enabled);
		}
		static inline cgss::Vector3 sensor_get_value(cgss::SensorType type) {
			if (!sensor_in_range(type)) return {};
			const auto v = sf::Sensor::getValue(static_cast<sf::Sensor::Type>(type));
			return { v.x, v.y, v.z };
		}

	private:
		// cgss::GamepadButton <-> sf::Joystick raw button index. SFML treats
		// joystick buttons as opaque indices; this mapping follows the
		// common XInput-style layout (SDL GameController convention) as seen
		// on Linux with Xbox / DualShock / Nintendo Switch Pro controllers.
		// Dpad buttons (LeftFace*) are reported as PovX/PovY axes on XInput,
		// not buttons — they return -1 (unmapped, false).
		static inline int to_sfml_joystick_button(cgss::GamepadButton b) noexcept {
			switch (b) {
				case cgss::GamepadButton::RightFaceDown:  return 0; // A / Cross
				case cgss::GamepadButton::RightFaceRight: return 1; // B / Circle
				case cgss::GamepadButton::RightFaceLeft:  return 2; // X / Square
				case cgss::GamepadButton::RightFaceUp:    return 3; // Y / Triangle
				case cgss::GamepadButton::LeftTrigger1:   return 4; // LB
				case cgss::GamepadButton::RightTrigger1:  return 5; // RB
				case cgss::GamepadButton::MiddleLeft:     return 6; // Back / Select
				case cgss::GamepadButton::MiddleRight:    return 7; // Start
				case cgss::GamepadButton::LeftThumb:      return 8; // LSB
				case cgss::GamepadButton::RightThumb:     return 9; // RSB
				case cgss::GamepadButton::Middle:         return 10; // Guide
				// LeftFace* (dpad), LeftTrigger2/RightTrigger2 (analog as axes)
				// and Unknown are not direct button indices under XInput.
				default: return -1;
			}
		}
		static inline cgss::GamepadButton from_sfml_joystick_button(unsigned int idx) noexcept {
			switch (idx) {
				case 0:  return cgss::GamepadButton::RightFaceDown;
				case 1:  return cgss::GamepadButton::RightFaceRight;
				case 2:  return cgss::GamepadButton::RightFaceLeft;
				case 3:  return cgss::GamepadButton::RightFaceUp;
				case 4:  return cgss::GamepadButton::LeftTrigger1;
				case 5:  return cgss::GamepadButton::RightTrigger1;
				case 6:  return cgss::GamepadButton::MiddleLeft;
				case 7:  return cgss::GamepadButton::MiddleRight;
				case 8:  return cgss::GamepadButton::LeftThumb;
				case 9:  return cgss::GamepadButton::RightThumb;
				case 10: return cgss::GamepadButton::Middle;
				default: return cgss::GamepadButton::Unknown;
			}
		}
		// cgss::GamepadAxis <-> sf::Joystick::Axis. X/Y = left stick,
		// U/V = right stick (XInput convention on Linux), Z/R = triggers.
		static inline sf::Joystick::Axis to_sfml_joystick_axis(cgss::GamepadAxis a) noexcept {
			switch (a) {
				case cgss::GamepadAxis::LeftX:        return sf::Joystick::X;
				case cgss::GamepadAxis::LeftY:        return sf::Joystick::Y;
				case cgss::GamepadAxis::RightX:       return sf::Joystick::U;
				case cgss::GamepadAxis::RightY:       return sf::Joystick::V;
				case cgss::GamepadAxis::LeftTrigger:  return sf::Joystick::Z;
				case cgss::GamepadAxis::RightTrigger: return sf::Joystick::R;
				case cgss::GamepadAxis::DpadX:        return sf::Joystick::PovX;
				case cgss::GamepadAxis::DpadY:        return sf::Joystick::PovY;
			}
			return sf::Joystick::X;
		}
		static inline cgss::GamepadAxis from_sfml_joystick_axis(sf::Joystick::Axis a) noexcept {
			switch (a) {
				case sf::Joystick::X: return cgss::GamepadAxis::LeftX;
				case sf::Joystick::Y: return cgss::GamepadAxis::LeftY;
				case sf::Joystick::U: return cgss::GamepadAxis::RightX;
				case sf::Joystick::V: return cgss::GamepadAxis::RightY;
				case sf::Joystick::Z: return cgss::GamepadAxis::LeftTrigger;
				case sf::Joystick::R: return cgss::GamepadAxis::RightTrigger;
				case sf::Joystick::PovX: return cgss::GamepadAxis::DpadX;
				case sf::Joystick::PovY: return cgss::GamepadAxis::DpadY;
			}
			return cgss::GamepadAxis::LeftX;
		}
	public:

		// Desktop-mode static query (independent of any window instance).
		static inline std::vector<cgss::VideoMode> fullscreen_modes() {
			std::vector<cgss::VideoMode> modes;
			for (const auto& m : sf::VideoMode::getFullscreenModes()) {
				modes.push_back({ m.width, m.height, m.bitsPerPixel });
			}
			return modes;
		}
		static inline unsigned int texture_max_size() {
			return sf::Texture::getMaximumSize();
		}
		static inline Size2u desktop_size() noexcept {
			const auto m = sf::VideoMode::getDesktopMode();
			return { m.width, m.height };
		}
		static inline void window_set_icon(native_window& w, unsigned int width, unsigned int height, const std::uint8_t* pixels) {
			w.setIcon(width, height, pixels);
		}
		// File-path icon loader. Each backend owns the decoder so ext/
		// doesn't have to drag in a format-specific library. On failure
		// (unreadable / unsupported format) the icon is left unchanged.
		static inline void window_set_icon_from_file(native_window& w, const char* path) {
			sf::Image img;
			if (!img.loadFromFile(path)) return;
			const auto size = img.getSize();
			w.setIcon(size.x, size.y, img.getPixelsPtr());
		}
		// Creates / re-creates the SFML window with the given options + title.
		static inline void window_create(native_window& w, const WindowCreateOptions& opts, const char* title) {
			const auto style = opts.fullscreen
				? sf::Style::Fullscreen
				: (sf::Style::Close | sf::Style::Titlebar);
			sf::ContextSettings sfCtx{};
			if (opts.context != nullptr) {
				sfCtx = sf::ContextSettings(
					opts.context->depthBits, opts.context->stencilBits,
					opts.context->antialiasingLevel, opts.context->majorVersion,
					opts.context->minorVersion, opts.context->attributeFlags,
					opts.context->sRgbCapable);
			}
			w.create(
				sf::VideoMode { opts.width, opts.height, opts.bitsPerPixel },
				title,
				style,
				sfCtx);
			if (opts.hidden) w.setVisible(false);
		}
		// Returns a render-target reference that draws to the window. Under
		// SFML this is just an upcast (sf::RenderWindow IS-A sf::RenderTarget).
		// Other backends may stash a tag and dispatch their draw ops on it.
		static inline native_render_target& target_from_window(native_window& w) noexcept {
			return w;
		}
		// Draws the render texture's color buffer onto the window (used by
		// post-processing). Encapsulates the SFML-specific local-sprite
		// construction so the wrapper body stays backend-portable.
		static inline void window_draw_render_texture(native_window& w,
		                                              const native_render_texture& rt,
		                                              const native_render_states* states) {
			sf::Sprite sp(rt.getTexture());
			if (states == nullptr) {
				w.draw(sp);
			} else {
				w.draw(sp, *states);
			}
		}
		// Draws a single solid-color quad covering the given rect on the
		// window (used by the brightness overlay).
		static inline void window_draw_color_quad(native_window& w,
		                                          float x, float y, float width, float height,
		                                          cgss::Color color) {
			const auto sfColor = sf::Color(color.r, color.g, color.b, color.a);
			const sf::Vertex vertices[4] = {
				sf::Vertex { sf::Vector2f(x,         y),          sfColor, sf::Vector2f(0, 0) },
				sf::Vertex { sf::Vector2f(x,         y + height), sfColor, sf::Vector2f(0, 0) },
				sf::Vertex { sf::Vector2f(x + width, y),          sfColor, sf::Vector2f(0, 0) },
				sf::Vertex { sf::Vector2f(x + width, y + height), sfColor, sf::Vector2f(0, 0) },
			};
			w.draw(vertices, 4, sf::PrimitiveType::TriangleStrip);
		}

		// ---- Render texture ops ----
		static inline bool render_texture_create(native_render_texture& rt, unsigned int w, unsigned int h) {
			return rt.create(w, h);
		}
		// Depth-aware creation — used by Viewport::setDepth3D and other
		// consumers that plan to composite 3D content into a 2D pipeline.
		// SFML 2.6's sf::RenderTexture.create(w, h, settings) accepts a
		// sf::ContextSettings carrying the depth-buffer bit count; any
		// non-zero value allocates a depth attachment. Mapping cgss's
		// boolean flag to a typical 24-bit depth buffer is sufficient for
		// the 2D-composition use case; real 3D backends (bgfx) will add
		// finer-grained Ops later per the umbrella plan's §6b.
		static inline bool render_texture_create_with_depth(native_render_texture& rt, unsigned int w, unsigned int h, bool with_depth) {
			if (!with_depth) return rt.create(w, h);
			sf::ContextSettings ctx{};
			ctx.depthBits   = 24;
			ctx.stencilBits = 8;
			return rt.create(w, h, ctx);
		}
		static inline void render_texture_set_smooth(native_render_texture& rt, bool smooth) {
			rt.setSmooth(smooth);
		}
		static inline const native_texture& render_texture_get_texture(const native_render_texture& rt) {
			return rt.getTexture();
		}
		static inline void render_texture_set_view(native_render_texture& rt, const native_view& v) {
			rt.setView(v);
		}
		static inline void render_texture_display(native_render_texture& rt) {
			rt.display();
		}
		static inline void render_texture_clear(native_render_texture& rt) {
			rt.clear();
		}
		static inline void render_texture_clear_color(native_render_texture& rt, cgss::Color c) {
			rt.clear(sf::Color(c.r, c.g, c.b, c.a));
		}

		// ---- View ops ----
		static inline Size2f view_get_size(const native_view& v) noexcept {
			const auto s = v.getSize();
			return { s.x, s.y };
		}
		static inline void view_set_size(native_view& v, float w, float h) noexcept {
			v.setSize(w, h);
		}
		static inline Size2f view_get_center(const native_view& v) noexcept {
			const auto c = v.getCenter();
			return { c.x, c.y };
		}
		static inline void view_set_center(native_view& v, float x, float y) noexcept {
			v.setCenter(x, y);
		}
		static inline Box2f view_get_viewport(const native_view& v) noexcept {
			const auto r = v.getViewport();
			return { r.left, r.top, r.width, r.height };
		}
		static inline void view_set_viewport(native_view& v, float left, float top, float width, float height) noexcept {
			v.setViewport(sf::FloatRect(left, top, width, height));
		}
		static inline void view_reset(native_view& v, float left, float top, float width, float height) noexcept {
			v.reset(sf::FloatRect(left, top, width, height));
		}
		static inline void view_zoom(native_view& v, float factor) noexcept {
			v.zoom(factor);
		}
		static inline void view_set_rotation(native_view& v, float angle) noexcept {
			v.setRotation(angle);
		}

		// ---- Draw-state ops ----
		// Native shader pointer is owned by basic_RenderStatesData<B>; this
		// op just stores the raw pointer into the native render states.
		static inline void draw_state_set_shader(native_render_states& s, native_shader* shader) noexcept {
			s.shader = shader;
		}
		static inline void draw_state_set_blend_factor(native_render_states& s, BlendFactorSlot slot, cgss::BlendFactor f) noexcept {
			const auto v = static_cast<sf::BlendMode::Factor>(f);
			switch (slot) {
				case BlendFactorSlot::ColorSrc: s.blendMode.colorSrcFactor = v; break;
				case BlendFactorSlot::ColorDst: s.blendMode.colorDstFactor = v; break;
				case BlendFactorSlot::AlphaSrc: s.blendMode.alphaSrcFactor = v; break;
				case BlendFactorSlot::AlphaDst: s.blendMode.alphaDstFactor = v; break;
			}
		}
		static inline cgss::BlendFactor draw_state_get_blend_factor(const native_render_states& s, BlendFactorSlot slot) noexcept {
			sf::BlendMode::Factor v = sf::BlendMode::Factor::Zero;
			switch (slot) {
				case BlendFactorSlot::ColorSrc: v = s.blendMode.colorSrcFactor; break;
				case BlendFactorSlot::ColorDst: v = s.blendMode.colorDstFactor; break;
				case BlendFactorSlot::AlphaSrc: v = s.blendMode.alphaSrcFactor; break;
				case BlendFactorSlot::AlphaDst: v = s.blendMode.alphaDstFactor; break;
			}
			return static_cast<cgss::BlendFactor>(v);
		}
		static inline void draw_state_set_blend_equation(native_render_states& s, BlendEquationSlot slot, cgss::BlendEquation e) noexcept {
			const auto v = static_cast<sf::BlendMode::Equation>(e);
			switch (slot) {
				case BlendEquationSlot::Color: s.blendMode.colorEquation = v; break;
				case BlendEquationSlot::Alpha: s.blendMode.alphaEquation = v; break;
			}
		}
		static inline cgss::BlendEquation draw_state_get_blend_equation(const native_render_states& s, BlendEquationSlot slot) noexcept {
			sf::BlendMode::Equation v = sf::BlendMode::Equation::Add;
			switch (slot) {
				case BlendEquationSlot::Color: v = s.blendMode.colorEquation; break;
				case BlendEquationSlot::Alpha: v = s.blendMode.alphaEquation; break;
			}
			return static_cast<cgss::BlendEquation>(v);
		}

		// ---- Font ops ----
		static inline bool font_load_from_file(native_font& f, const char* filename) {
			return f.loadFromFile(filename);
		}
		// SFML rasterizes per-size on demand from the loaded face — the size
		// hint here is unused. Provided for Ops symmetry with raylib, which
		// must rebuild its atlas at the target size to avoid downscaling
		// pixel fonts.
		static inline bool font_load_from_file_with_size(native_font& f, const char* filename, int /*fontSize*/) {
			return f.loadFromFile(filename);
		}
		static inline bool font_load_from_memory(native_font& f, const void* data, std::size_t size) {
			return f.loadFromMemory(data, size);
		}
		static inline bool font_load_from_memory_with_size(native_font& f, const void* data, std::size_t size, int /*fontSize*/) {
			return f.loadFromMemory(data, size);
		}
		// No-op on SFML — sf::Font's dtor releases its font-face data via
		// its own unique_ptr<sf::Font> deleter. Exposed for Ops-symmetry
		// with RaylibBackend::Ops::font_destroy, which must call UnloadFont
		// on the POD.
		static inline void font_destroy(native_font&) noexcept {}

		// ---- Shader ops ----
		// Capability queries.
		static inline bool shader_is_available()          { return sf::Shader::isAvailable(); }
		static inline bool shader_is_geometry_available() { return sf::Shader::isGeometryAvailable(); }

		// Whether the current GL context accepts the SFML legacy fixed-pipeline
		// GLSL contract (gl_FragColor / gl_TexCoord[0] / texture2D / gl_Color).
		// SFML always creates a compat-profile context where these work, so
		// this is unconditionally true for the SFML backend. PSDK reads this
		// to decide whether to emit legacy or modern shader sources.
		static inline bool shader_supports_fixed_pipeline() { return true; }

		// Preprocessor target: desktop SFML feeds the fixed-pipeline built-ins;
		// SFML ES (the SFML-opengl-es2 fork, LITECGSS_USE_FIXED_PIPELINE off)
		// needs them rewritten onto its attributes, in GLSL ES 1.00.
		static constexpr cgss::ShaderPreprocessor::Target shader_preprocessor_target() noexcept {
#ifdef LITECGSS_USE_FIXED_PIPELINE
			return cgss::ShaderPreprocessor::Target::DesktopFixedPipeline;
#else
			return cgss::ShaderPreprocessor::Target::SfmlEs100;
#endif
		}

		// Whether textures are stored at their size, or padded to the next
		// power of two (GL ES 2 without OES_texture_npot, SFML fork only).
		static inline bool texture_supports_npot() { return sfml_caps::isNonPowerOfTwoSupported(); }

		// Compile from in-memory source. The 1-arg-typed form takes the cgss
		// stage enum (Vertex / Fragment / Geometry); the 2-arg form is
		// vertex+fragment; the 3-arg form is vertex+geometry+fragment.
		static inline bool shader_load_from_memory(native_shader& sh, const std::string& src, cgss::ShaderType type) {
			return sh.loadFromMemory(src, static_cast<sf::Shader::Type>(type));
		}
		static inline bool shader_load_from_memory(native_shader& sh, const std::string& vsrc, const std::string& fsrc) {
			return sh.loadFromMemory(vsrc, fsrc);
		}
		static inline bool shader_load_from_memory(native_shader& sh, const std::string& vsrc, const std::string& gsrc, const std::string& fsrc) {
			return sh.loadFromMemory(vsrc, gsrc, fsrc);
		}

		// Uniform setters — overload set on the value type.
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, float v) { sh.setUniform(name, v); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, int v)   { sh.setUniform(name, v); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, bool v)  { sh.setUniform(name, v); }

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec2& v) { sh.setUniform(name, sf::Glsl::Vec2(v.x, v.y)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec3& v) { sh.setUniform(name, sf::Glsl::Vec3(v.x, v.y, v.z)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Vec4& v) { sh.setUniform(name, sf::Glsl::Vec4(v.x, v.y, v.z, v.w)); }

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec2& v) { sh.setUniform(name, sf::Glsl::Ivec2(v.x, v.y)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec3& v) { sh.setUniform(name, sf::Glsl::Ivec3(v.x, v.y, v.z)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Ivec4& v) { sh.setUniform(name, sf::Glsl::Ivec4(v.x, v.y, v.z, v.w)); }

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec2& v) { sh.setUniform(name, sf::Glsl::Bvec2(v.x, v.y)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec3& v) { sh.setUniform(name, sf::Glsl::Bvec3(v.x, v.y, v.z)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Bvec4& v) { sh.setUniform(name, sf::Glsl::Bvec4(v.x, v.y, v.z, v.w)); }

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Mat3& m) { sh.setUniform(name, sf::Glsl::Mat3(m.array)); }
		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const cgss::Glsl::Mat4& m) { sh.setUniform(name, sf::Glsl::Mat4(m.array)); }

		static inline void shader_set_uniform(native_shader& sh, const std::string& name, const native_texture& t) { sh.setUniform(name, t); }

		// Bound-active-texture sentinel — separate name (not an overload of the
		// value setter) because the cgss tag type carries no payload to dispatch
		// on alongside the other overloads cleanly.
		static inline void shader_set_uniform_current_texture(native_shader& sh, const std::string& name) {
			sh.setUniform(name, sf::Shader::CurrentTexture);
		}

		static inline void shader_set_uniform_array(native_shader& sh, const std::string& name, const float* arr, std::size_t n) {
			sh.setUniformArray(name, arr, n);
		}

		// ---- Render-target view ops ----
		// View is returned by value (a copy) so callers can stash it and
		// restore later via target_set_view.
		static inline void target_set_view(native_render_target& target, const native_view& view) {
			target.setView(view);
		}
		static inline native_view target_get_view(const native_render_target& target) {
			return target.getView();
		}
		static inline native_view target_get_default_view(const native_render_target& target) {
			return target.getDefaultView();
		}

		// ---- Draw thunks ----
		static inline void draw_sprite(native_render_target& target,
		                               const native_sprite& sprite,
		                               const native_render_states& states) {
			target.draw(sprite, states);
		}

		// ScrollText is a sf::Drawable on SFML; one-line forwarder.
		static inline void draw_text(native_render_target& target, const cgss::ScrollText& text) {
			target.draw(text);
		}

		// Draws the sprite with a caller-prebuilt transform composed in,
		// optionally over a base render state (for shaders). Callers (Sprite /
		// Shape items) cache the `native_transform` keyed by
		// `Transformable::transformVersion()` and only rebuild it via
		// `build_transform` when the transform state actually mutates.
		static inline void draw_sprite_with_transform(native_render_target& target,
		                                              const native_sprite& sprite,
		                                              const native_transform& xform,
		                                              const native_render_states* base_states) {
			sf::RenderStates states = base_states == nullptr ? sf::RenderStates::Default : *base_states;
			states.transform *= xform;
			target.draw(sprite, states);
		}

		// Shape counterpart of the above.
		static inline void draw_shape_with_transform(native_render_target& target,
		                                             const cgss::ShapeGeometry& g,
		                                             const native_texture* tex,
		                                             const native_transform& xform,
		                                             const native_render_states* base_states) {
			sf::RenderStates states = base_states == nullptr ? sf::RenderStates::Default : *base_states;
			states.transform *= xform;
			draw_shape(target, g, tex, states);
		}

		// ---- Sprite ops ----
		static inline cgss::Color get_sprite_color(const native_sprite& s) noexcept {
			const auto c = s.getColor();
			return cgss::Color{ c.r, c.g, c.b, c.a };
		}

		static inline void set_sprite_color(native_sprite& s, cgss::Color c) noexcept {
			s.setColor(sf::Color(c.r, c.g, c.b, c.a));
		}

		static inline cgss::IntRect get_sprite_texture_rect(const native_sprite& s) noexcept {
			const auto r = s.getTextureRect();
			return cgss::IntRect{ r.left, r.top, r.width, r.height };
		}

		static inline void set_sprite_texture_rect(native_sprite& s, const cgss::IntRect& r) noexcept {
			s.setTextureRect(sf::IntRect(r.left, r.top, r.width, r.height));
		}

		static inline void set_sprite_texture(native_sprite& s, const native_texture& t, bool resetRect) {
			s.setTexture(t, resetRect);
		}

		// Position used for raw sprite-handle drawing (e.g., the per-window
		// user-render sprite owned by DisplayWindow). cgss::Sprite stores its
		// own position in cgss::Transformable; this is for native handles.
		static inline void sprite_set_position(native_sprite& s, float x, float y) {
			s.setPosition(x, y);
		}

		// Origin / scale on raw native-sprite handles (used by SpriteMap's
		// per-tile sprites, which are owned outside cgss::Sprite).
		static inline void sprite_set_origin(native_sprite& s, float x, float y) {
			s.setOrigin(x, y);
		}
		static inline void sprite_set_scale(native_sprite& s, float sx, float sy) {
			s.setScale(sx, sy);
		}

		// Every sf::RenderWindow is its own OS window with its own GL
		// context, which SFML activates on the drawing thread by itself.
		static inline bool window_is_drawable(const native_window&) noexcept { return true; }
		static constexpr bool context_follows_thread = false;
		static inline void context_make_current(bool) {}

		// No-states draw overload — falls back to sf::RenderStates::Default.
		static inline void draw_sprite(native_render_target& target, const native_sprite& sprite) {
			target.draw(sprite);
		}

		// Draws `count` sprites sharing one texture in a single draw call: two
		// triangles per sprite, laid out as sf::Sprite lays its quad out.
		static inline void draw_sprite_batch(native_render_target& target,
		                                     const native_sprite* const* sprites, std::size_t count) {
			if (count == 0) return;
			if (count == 1) {
				// sf::Sprite's own draw (a cached triangle strip) is as cheap
				target.draw(*sprites[0]);
				return;
			}
			thread_local std::vector<sf::Vertex> vertices;
			vertices.clear();
			vertices.reserve(count * 6);
			for (std::size_t i = 0; i < count; ++i) {
				const auto& sprite = *sprites[i];
				const auto& t = sprite.getTransform();
				const auto& rect = sprite.getTextureRect();
				const auto w = static_cast<float>(std::abs(rect.width));
				const auto h = static_cast<float>(std::abs(rect.height));
				const auto color = sprite.getColor();
				const auto u0 = static_cast<float>(rect.left);
				const auto v0 = static_cast<float>(rect.top);
				const auto u1 = u0 + static_cast<float>(rect.width);
				const auto v1 = v0 + static_cast<float>(rect.height);
				const sf::Vertex p0{ t.transformPoint(0.f, 0.f), color, { u0, v0 } };
				const sf::Vertex p1{ t.transformPoint(w,   0.f), color, { u1, v0 } };
				const sf::Vertex p2{ t.transformPoint(0.f, h),   color, { u0, v1 } };
				const sf::Vertex p3{ t.transformPoint(w,   h),   color, { u1, v1 } };
				vertices.insert(vertices.end(), { p0, p1, p2, p1, p3, p2 });
			}
			sf::RenderStates states;
			states.texture = sprites[0]->getTexture();
			target.draw(vertices.data(), vertices.size(), sf::Triangles, states);
		}

		// Returns the texture's size if a texture is bound, else {0, 0}.
		static inline Size2u get_sprite_bound_texture_size(const native_sprite& s) noexcept {
			const auto* t = s.getTexture();
			if (t == nullptr) return { 0u, 0u };
			const auto sz = t->getSize();
			return { sz.x, sz.y };
		}

		// ---- Shape ops ----
		// Builds a temporary native shape primitive matching the geometry
		// type, applies the material state, and returns it via the callback
		// so callers can either draw it or query it without re-computing.
		template <class F>
		static inline auto with_native_shape(const cgss::ShapeGeometry& g, const native_texture* tex, F&& f) {
			auto setupCommon = [&](sf::Shape& s) {
				s.setFillColor(sf::Color(g.fillColor.r, g.fillColor.g, g.fillColor.b, g.fillColor.a));
				s.setOutlineColor(sf::Color(g.outlineColor.r, g.outlineColor.g, g.outlineColor.b, g.outlineColor.a));
				s.setOutlineThickness(g.outlineThickness);
				if (tex != nullptr) {
					s.setTexture(tex, /*resetRect=*/false);
				}
				if (g.textureRect.width != 0 || g.textureRect.height != 0) {
					s.setTextureRect(sf::IntRect(g.textureRect.left, g.textureRect.top, g.textureRect.width, g.textureRect.height));
				}
			};
			switch (g.type) {
				case cgss::ShapeType::Rectangle: {
					sf::RectangleShape s(sf::Vector2f(g.width, g.height));
					setupCommon(s);
					return f(s);
				}
				case cgss::ShapeType::Circle: {
					sf::CircleShape s(g.radius, g.pointCount);
					setupCommon(s);
					return f(s);
				}
				case cgss::ShapeType::Convex: {
					sf::ConvexShape s(g.pointCount);
					for (std::size_t i = 0; i < g.points.size(); ++i) {
						s.setPoint(i, sf::Vector2f(g.points[i].x, g.points[i].y));
					}
					setupCommon(s);
					return f(s);
				}
			}
			// Unreachable.
			sf::RectangleShape s; return f(s);
		}

		static inline void draw_shape(native_render_target& target,
		                              const cgss::ShapeGeometry& g,
		                              const native_texture* tex,
		                              const native_render_states& states) {
			with_native_shape(g, tex, [&](const sf::Shape& s) { target.draw(s, states); });
		}

		static inline cgss::Point get_shape_point(const cgss::ShapeGeometry& g, std::size_t index) {
			return with_native_shape(g, nullptr, [&](const sf::Shape& s) {
				const auto p = s.getPoint(index);
				return cgss::Point{ p.x, p.y };
			});
		}

		// Bounds of the shape with the cgss::Transformable applied (matches
		// SFML's getGlobalBounds semantics — used by Shape::getWidth/getHeight).
		struct Bounds2f { float width; float height; };
		static inline Bounds2f get_shape_global_bounds(const cgss::ShapeGeometry& g, const cgss::Transformable& xf) {
			return with_native_shape(g, nullptr, [&](sf::Shape& s) {
				s.setPosition(xf.getX(), xf.getY());
				s.setRotation(-xf.getAngle());
				s.setScale(xf.getScaleX(), xf.getScaleY());
				s.setOrigin(xf.getOx(), xf.getOy());
				const auto b = s.getGlobalBounds();
				return Bounds2f{ b.width, b.height };
			});
		}

		// ---- Texture / Image ops ----
		static inline void set_smooth(native_texture& t, bool smooth) noexcept {
			t.setSmooth(smooth);
		}

		static inline Size2u get_texture_size(const native_texture& t) noexcept {
			const auto sz = t.getSize();
			return { sz.x, sz.y };
		}

		static inline Size2u get_image_size(const native_image& i) noexcept {
			const auto sz = i.getSize();
			return { sz.x, sz.y };
		}

		static inline bool create_texture(native_texture& t, unsigned int width, unsigned int height) {
			return t.create(width, height);
		}

		static inline void update_texture_from_image(native_texture& t, const native_image& i) {
			t.update(i);
		}

		// Upload a contiguous RGBA pixel buffer into a sub-rect of the texture.
		// Used for streaming-decoded sources (YukiGif).
		static inline void texture_update_pixels(native_texture& t, const std::uint8_t* pixels,
		                                        unsigned int width, unsigned int height,
		                                        unsigned int x, unsigned int y) {
			t.update(pixels, width, height, x, y);
		}

		// Create a GPU-backed texture from a CPU-side image (SFML's
		// loadFromImage allocates GPU storage + uploads). Used by
		// ext/Sprite to move bitmap data onto the GPU.
		// false when the GPU refuses the texture (larger than its maximum size)
		static inline bool texture_create_from_image(native_texture& t, const native_image& i) {
			return t.loadFromImage(i);
		}

		// Release a texture's backing. sf::Texture's own destructor frees
		// GPU memory, but when reassigning an existing sf::Texture (e.g.
		// the ext Sprite swapping bitmaps) we need a fresh empty state.
		// SFML has no explicit "unload" — construct-and-swap does the job.
		static inline void texture_destroy(native_texture& t) noexcept {
			t = native_texture{};
		}

		// ---- Viewport scope: scissor + transform push/pop (SFML emulation) ----
		// SFML has no native scissor — approximated via sf::View's viewport
		// (normalized [0,1] rect on the render target). Transform is baked
		// into the view's center/size. A file-static view stack supports
		// nested viewport_begin calls (matches raylib's rlPushMatrix stack).
		//
		// Thread-safety: single-threaded render only, matching raylib.
	private:
		static inline std::vector<sf::View>& viewStack() {
			static std::vector<sf::View> stack;
			return stack;
		}
	public:
		static inline void viewport_begin(native_render_target& target,
		                                  int clipX, int clipY, int clipW, int clipH,
		                                  float offsetX, float offsetY,
		                                  float scaleX, float scaleY) {
			viewStack().push_back(target.getView());
			const auto size = target.getSize();
			const float fw = static_cast<float>(size.x);
			const float fh = static_cast<float>(size.y);
			sf::View v;
			// Viewport (normalized [0,1] rect on the render target) — clips
			// drawing to the intended window-pixel region.
			v.setViewport(sf::FloatRect(
				fw > 0 ? static_cast<float>(clipX) / fw : 0.f,
				fh > 0 ? static_cast<float>(clipY) / fh : 0.f,
				fw > 0 ? static_cast<float>(clipW) / fw : 1.f,
				fh > 0 ? static_cast<float>(clipH) / fh : 1.f));
			// Content size in world coordinates so that (0,0)..(clipW,clipH)
			// maps 1:1 in window pixels, then scale inversely to match
			// raylib's rlScalef semantics (rlScalef zooms content *into*
			// the viewport; a larger scale means larger visible content).
			const float worldW = scaleX > 0.f ? static_cast<float>(clipW) / scaleX : static_cast<float>(clipW);
			const float worldH = scaleY > 0.f ? static_cast<float>(clipH) / scaleY : static_cast<float>(clipH);
			v.setSize(worldW, worldH);
			// Center so that the top-left of the visible area is at
			// (-offsetX/scaleX, -offsetY/scaleY) in world space — matches
			// rlTranslatef before rlScalef in raylib.
			v.setCenter(-offsetX / (scaleX > 0 ? scaleX : 1.f) + worldW * 0.5f,
			            -offsetY / (scaleY > 0 ? scaleY : 1.f) + worldH * 0.5f);
			target.setView(v);
		}
		static inline void viewport_end(native_render_target& target) {
			auto& stack = viewStack();
			if (!stack.empty()) {
				target.setView(stack.back());
				stack.pop_back();
			}
		}

		// Filled rect fill. SFML: sf::RectangleShape drawn on the target.
		static inline void draw_filled_rect(native_render_target& target,
		                                    int x, int y, int w, int h, cgss::Color c) {
			sf::RectangleShape r(sf::Vector2f(static_cast<float>(w), static_cast<float>(h)));
			r.setPosition(static_cast<float>(x), static_cast<float>(y));
			r.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
			target.draw(r);
		}

		// Immediate-mode textured draw — SFML emulates raylib's DrawTexturePro
		// via sf::Sprite (configured + drawn through the render target).
		//
		// Negative `srcRect.width` encodes horizontal mirror — sf::IntRect
		// supports negative dimensions too, matching the raylib convention.
		//
		// Origin mapping: raylib's DrawTexturePro takes origin in destination-
		// pixel coordinates (post-scale). sf::Sprite's setOrigin is in
		// texture-pixel coordinates (pre-scale). Convert: `sfOrigin =
		// rlOrigin / scale` where `scale = dstW / |srcW|`.
		static inline void draw_texture_pro(
				native_render_target& target,
				const native_texture& tex,
				const cgss::IntRect& srcRect,
				float dstX, float dstY, float dstWidth, float dstHeight,
				float originX, float originY,
				float angleDegrees, cgss::Color tint) {
			sf::Sprite sprite(tex);
			sprite.setTextureRect(sf::IntRect(srcRect.left, srcRect.top,
			                                  srcRect.width, srcRect.height));
			const float absSrcW = srcRect.width  < 0 ? -static_cast<float>(srcRect.width)  : static_cast<float>(srcRect.width);
			const float absSrcH = srcRect.height < 0 ? -static_cast<float>(srcRect.height) : static_cast<float>(srcRect.height);
			const float scaleX = absSrcW > 0.f ? dstWidth  / absSrcW : 1.f;
			const float scaleY = absSrcH > 0.f ? dstHeight / absSrcH : 1.f;
			sprite.setScale(scaleX, scaleY);
			sprite.setOrigin(scaleX != 0.f ? originX / scaleX : 0.f,
			                 scaleY != 0.f ? originY / scaleY : 0.f);
			sprite.setPosition(dstX, dstY);
			sprite.setRotation(angleDegrees);
			sprite.setColor(sf::Color(tint.r, tint.g, tint.b, tint.a));
			target.draw(sprite);
		}

		static inline void copy_image(native_image& dst, const native_image& src) {
			dst = src;
		}

		static inline void image_set_pixel(native_image& i, unsigned int x, unsigned int y, cgss::Color c) {
			i.setPixel(x, y, sf::Color(c.r, c.g, c.b, c.a));
		}

		static inline cgss::Color image_get_pixel(const native_image& i, unsigned int x, unsigned int y) {
			const auto c = i.getPixel(x, y);
			return cgss::Color{ c.r, c.g, c.b, c.a };
		}

		static inline void image_blit(native_image& dst, unsigned int x, unsigned int y,
		                              const native_image& src, const cgss::IntRect& srcRect,
		                              bool applyAlpha = false) {
			dst.copy(src, x, y, sf::IntRect(srcRect.left, srcRect.top, srcRect.width, srcRect.height), applyAlpha);
		}

		static inline const std::uint8_t* get_image_pixels_ptr(const native_image& i) noexcept {
			return i.getPixelsPtr();
		}

		static inline void image_create_with_pixels(native_image& i, unsigned int w, unsigned int h, const std::uint8_t* pixels) {
			i.create(w, h, pixels);
		}

		static inline void image_create_mask_from_color(native_image& i, cgss::Color color, std::uint8_t alpha) {
			i.createMaskFromColor(sf::Color(color.r, color.g, color.b, color.a), alpha);
		}
		// No-op on SFML — sf::Image's own destructor (called when its
		// unique_ptr<sf::Image> deleter runs) handles pixel release. This
		// Op exists for Ops-symmetry with RaylibBackend::Ops::image_destroy,
		// which must explicitly call UnloadImage on the POD.
		static inline void image_destroy(native_image&) noexcept {}

		// PNG codec layer for the unified Image/Texture serializers.
		// SFML side decodes via sf::Image::loadFromMemory / loadFromFile
		// (SFML uses stb-image internally) and encodes via lodepng so we
		// keep deterministic byte-for-byte output.
		static inline bool png_decode_memory(native_image& image, const void* data, std::size_t size) {
			return image.loadFromMemory(data, size);
		}

		static inline bool png_decode_file(native_image& image, const char* filename) {
			return image.loadFromFile(filename);
		}

		// Returns a heap-allocated PNG buffer (caller frees with std::free).
		// MemorySerializerData lifecycle expects std::free in the saver path.
		static inline cgss::MemorySerializerData png_encode_memory(const native_image& image) {
			unsigned char* out = nullptr;
			std::size_t size = 0;
			if (lodepng_encode32(&out, &size, image.getPixelsPtr(), image.getSize().x, image.getSize().y) != 0) {
				if (out != nullptr) std::free(out);
				return { nullptr, 0u };
			}
			return { out, size };
		}

		// Returns 0 on success — matches lodepng_encode32_file's contract.
		static inline unsigned int png_encode_file(const native_image& image, const char* filename) {
			return lodepng_encode32_file(filename, image.getPixelsPtr(), image.getSize().x, image.getSize().y);
		}

		// Allocate an empty RGBA image (all pixels transparent). Used by
		// the EmptySerializeMethod template specialisations.
		static inline bool image_create_blank_rgba(native_image& image, unsigned int width, unsigned int height) {
			image.create(width, height, sf::Color(0, 0, 0, 0));
			return true;
		}

		// Read GPU texels back to a CPU image (used by the texture saver).
		static inline bool texture_read_to_image(const native_texture& t, native_image& image) {
			image = t.copyToImage();
			return true;
		}
	};

	// Free-function helper that forwards to the active backend's
	// Ops::native_of. Lets impl .cpps write `cgss::backend::native_of(target)`
	// (or `native_of(target)` via ADL since wrappers live in `cgss` and the
	// active backend in `cgss::backend`) instead of fully-qualifying the
	// per-backend Ops path.
	template <class W>
	inline auto& native_of(W& w) noexcept {
		return SFMLBackend::Ops::native_of(w);
	}
	template <class W>
	inline const auto& native_of(const W& w) noexcept {
		return SFMLBackend::Ops::native_of(w);
	}

}

#endif
