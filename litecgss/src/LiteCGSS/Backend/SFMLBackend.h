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
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>

#include "LiteCGSS/Backend/Internal/SFMLConversions.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Transformable.h"

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
		// Desktop-mode static query (independent of any window instance).
		static inline Size2u desktop_size() noexcept {
			const auto m = sf::VideoMode::getDesktopMode();
			return { m.width, m.height };
		}
		static inline void window_set_icon(native_window& w, unsigned int width, unsigned int height, const std::uint8_t* pixels) {
			w.setIcon(width, height, pixels);
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
		static inline bool font_load_from_memory(native_font& f, const void* data, std::size_t size) {
			return f.loadFromMemory(data, size);
		}

		// ---- Shader ops ----
		// Capability queries.
		static inline bool shader_is_available()          { return sf::Shader::isAvailable(); }
		static inline bool shader_is_geometry_available() { return sf::Shader::isGeometryAvailable(); }

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

		// No-states draw overload — falls back to sf::RenderStates::Default.
		static inline void draw_sprite(native_render_target& target, const native_sprite& sprite) {
			target.draw(sprite);
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
