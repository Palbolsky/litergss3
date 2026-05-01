#include "DisplayWindow.h"
#include "LiteRGSS.h"
#include "../events/Events.h"
#include "../graphics/BlendMode.h"
#include "../graphics/Image.h"
#include "../rbAdapter.h"

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Events/Event.h"
#include "LiteCGSS/Image/Image.h"
#include "LiteCGSS/Views/DisplayWindow.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops     = Backend::Ops;

	// File-static cgss::DisplayWindow. Owns the native window plus the
	// View-side DrawableStack that Sprite/Viewport/Text/Shape/SpriteMap
	// register into (via cgss::*::create). cgss::DisplayWindow::draw() runs
	// the atomic clear → drawables → postProcessing → display sequence.
	std::unique_ptr<cgss::DisplayWindow> g_window;

	// Per-button last-pressed state used to derive press/release edges from
	// LiteRGSS3's typed events (KeyEvent and MouseButtonEvent share one
	// Struct class, so the dispatch decides which on_* callback to fire by
	// diffing against the last polled state).
	std::unordered_map<int, bool> g_last_keys;
	std::unordered_map<int, bool> g_last_mouse;
}

VALUE rb_cDisplayWindow      = Qnil;
VALUE rb_eClosedWindowError  = Qnil;

// Singleton DisplayWindow Ruby instance — the LiteRGSS2 surface exposes
// `LiteRGSS::DisplayWindow.desktop_width` / `.desktop_height` as class
// methods, and several PSDK call sites read state via the class itself.
static VALUE g_display_window_instance = Qnil;

int base_width  = 640;
int base_height = 480;
double window_scale = 1.0;

cgss::DisplayWindow* get_active_display_window()
{
	return g_window.get();
}

static void ensureWindow()
{
	if (g_window == nullptr) {
		rb_raise(rb_eRGSSError, "Window not opened — call DisplayWindow.new first");
	}
}

// Settings array index helpers — must stay in sync with the Ruby
// `settings` getter below. The 9-tuple matches LiteRGSS2's layout.
enum SettingsIdx {
	SI_TITLE = 0, SI_WIDTH, SI_HEIGHT, SI_SCALE, SI_BPP,
	SI_FRAMERATE, SI_VSYNC, SI_FULLSCREEN, SI_VISIBLE_MOUSE,
	SI_COUNT
};

// Instance-state ivar names. Use ivars rather than DATA pointer storage so
// the existing struct-less DisplayWindow class layout doesn't need a
// custom alloc func.
static ID iv_settings;
static ID iv_brightness;
static ID iv_shader;
static ID iv_polling;
static ID iv_disposed;
static ID iv_focus;
static ID iv_event_callbacks;

// Forward declarations
static void apply_settings_array(VALUE self, VALUE arr);
static VALUE drain_events_internal(VALUE self);

// Build a DisplayWindowSettings from the current 9-tuple ivar.
static cgss::DisplayWindowSettings build_settings_from_array(VALUE arr)
{
	cgss::DisplayWindowSettings settings{};
	VALUE title         = rb_ary_entry(arr, SI_TITLE);
	VALUE width         = rb_ary_entry(arr, SI_WIDTH);
	VALUE height        = rb_ary_entry(arr, SI_HEIGHT);
	VALUE scale         = rb_ary_entry(arr, SI_SCALE);
	VALUE bpp           = rb_ary_entry(arr, SI_BPP);
	VALUE framerate     = rb_ary_entry(arr, SI_FRAMERATE);
	VALUE vsync         = rb_ary_entry(arr, SI_VSYNC);
	VALUE fullscreen    = rb_ary_entry(arr, SI_FULLSCREEN);
	VALUE visible_mouse = rb_ary_entry(arr, SI_VISIBLE_MOUSE);

	settings.title        = NIL_P(title) ? std::string{"LiteRGSS"} : std::string{StringValueCStr(title)};
	settings.video.width  = NUM2UINT(width);
	settings.video.height = NUM2UINT(height);
	settings.video.scale  = NUM2DBL(scale);
	settings.video.bitsPerPixel = NUM2UINT(bpp);
	settings.frameRate    = NUM2UINT(framerate);
	settings.vSync        = RTEST(vsync);
	settings.fullscreen   = RTEST(fullscreen);
	settings.visibleMouse = RTEST(visible_mouse);
	return settings;
}

static void open_window_with_settings(VALUE arr)
{
	auto settings = build_settings_from_array(arr);
	base_width  = static_cast<int>(settings.video.width);
	base_height = static_cast<int>(settings.video.height);
	window_scale = settings.video.scale;

	if (g_window == nullptr) {
		g_window = std::make_unique<cgss::DisplayWindow>(settings);
	} else {
		g_window->reload(std::move(settings), true);
	}
	g_last_keys.clear();
	g_last_mouse.clear();
}

// --- Construction ----------------------------------------------------------

// LiteRGSS2 9-arg constructor:
//   DisplayWindow.new(title, w, h, scale, bpp, framerate, vsync, fullscreen, visible_mouse)
// Defaults match LiteRGSS2's BuildSettings: bpp=32, framerate=60, vsync=true,
// fullscreen=false, visible_mouse=true.
VALUE rb_DisplayWindow_initialize(int argc, VALUE* argv, VALUE self)
{
	VALUE title, width, height, scale, bpp, framerate, vsync, fullscreen, visible_mouse;
	rb_scan_args(argc, argv, "09",
	             &title, &width, &height, &scale, &bpp,
	             &framerate, &vsync, &fullscreen, &visible_mouse);

	if (NIL_P(title))         title         = rb_utf8_str_new_cstr("LiteRGSS");
	if (NIL_P(width))         width         = INT2NUM(640);
	if (NIL_P(height))        height        = INT2NUM(480);
	if (NIL_P(scale))         scale         = DBL2NUM(1.0);
	if (NIL_P(bpp))           bpp           = INT2NUM(32);
	if (NIL_P(framerate))     framerate     = INT2NUM(60);
	if (NIL_P(vsync))         vsync         = Qtrue;
	if (NIL_P(fullscreen))    fullscreen    = Qfalse;
	if (NIL_P(visible_mouse)) visible_mouse = Qtrue;

	// Stash settings so subsequent `settings` reads roundtrip.
	VALUE arr = rb_ary_new_capa(SI_COUNT);
	rb_ary_push(arr, title);
	rb_ary_push(arr, width);
	rb_ary_push(arr, height);
	rb_ary_push(arr, scale);
	rb_ary_push(arr, bpp);
	rb_ary_push(arr, framerate);
	rb_ary_push(arr, vsync);
	rb_ary_push(arr, fullscreen);
	rb_ary_push(arr, visible_mouse);
	rb_ivar_set(self, iv_settings, arr);
	rb_ivar_set(self, iv_brightness, INT2NUM(255));
	rb_ivar_set(self, iv_shader, Qnil);
	rb_ivar_set(self, iv_polling, Qtrue);
	rb_ivar_set(self, iv_disposed, Qfalse);
	rb_ivar_set(self, iv_focus, Qtrue);
	rb_ivar_set(self, iv_event_callbacks, rb_hash_new());

	open_window_with_settings(arr);
	g_display_window_instance = self;
	return self;
}

// LiteRGSS2 alias: `DisplayWindow#open_window(width, height, title, scale)`.
// Multiple PSDK / test paths call this after `.new` to (re)configure the
// window. Accepts any prefix of the args; missing fields fall through to the
// values stashed during initialize.
static VALUE rb_DisplayWindow_open_window(int argc, VALUE* argv, VALUE self)
{
	VALUE width, height, title, scale;
	rb_scan_args(argc, argv, "04", &width, &height, &title, &scale);

	VALUE arr = rb_ivar_get(self, iv_settings);
	if (NIL_P(arr)) {
		arr = rb_ary_new_capa(SI_COUNT);
		for (int i = 0; i < SI_COUNT; ++i) rb_ary_push(arr, Qnil);
		rb_ivar_set(self, iv_settings, arr);
	}
	if (!NIL_P(width))  rb_ary_store(arr, SI_WIDTH,  width);
	if (!NIL_P(height)) rb_ary_store(arr, SI_HEIGHT, height);
	if (!NIL_P(title))  rb_ary_store(arr, SI_TITLE,  title);
	if (!NIL_P(scale))  rb_ary_store(arr, SI_SCALE,  scale);

	open_window_with_settings(arr);
	rb_ivar_set(self, iv_disposed, Qfalse);
	g_display_window_instance = self;
	return self;
}

// --- Settings round-trip ---------------------------------------------------

VALUE rb_DisplayWindow_getSettings(VALUE self)
{
	VALUE arr = rb_ivar_get(self, iv_settings);
	// Return a copy — PSDK mutates the array, then re-assigns via settings=.
	return rb_ary_dup(arr);
}

static void apply_settings_array(VALUE self, VALUE arr)
{
	Check_Type(arr, T_ARRAY);
	if (RARRAY_LEN(arr) != SI_COUNT) {
		rb_raise(rb_eTypeError, "Settings must be an array of %d values", SI_COUNT);
	}
	rb_ivar_set(self, iv_settings, rb_ary_dup(arr));
	if (g_window == nullptr) return;
	auto settings = build_settings_from_array(arr);
	base_width   = static_cast<int>(settings.video.width);
	base_height  = static_cast<int>(settings.video.height);
	window_scale = settings.video.scale;
	// reload(..., false) defers the reload to the next draw() — exactly the
	// behaviour we want when settings change mid-loop.
	g_window->reload(std::move(settings), false);
}

VALUE rb_DisplayWindow_setSettings(VALUE self, VALUE arr)
{
	apply_settings_array(self, arr);
	return arr;
}

VALUE rb_DisplayWindow_getWidth(VALUE self)
{
	return rb_ary_entry(rb_ivar_get(self, iv_settings), SI_WIDTH);
}

VALUE rb_DisplayWindow_getHeight(VALUE self)
{
	return rb_ary_entry(rb_ivar_get(self, iv_settings), SI_HEIGHT);
}

// --- Window operations -----------------------------------------------------

VALUE rb_DisplayWindow_scale(VALUE self, VALUE scale)
{
	ensureWindow();
	window_scale = NUM2DBL(scale);
	g_window->setScale(window_scale);
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_SCALE, scale);
	return self;
}

VALUE rb_DisplayWindow_resizeScreen(VALUE self, VALUE width, VALUE height)
{
	ensureWindow();
	base_width  = NUM2INT(width);
	base_height = NUM2INT(height);
	g_window->resizeScreen(static_cast<unsigned int>(base_width),
	                       static_cast<unsigned int>(base_height));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) {
		rb_ary_store(arr, SI_WIDTH, width);
		rb_ary_store(arr, SI_HEIGHT, height);
	}
	return self;
}

VALUE rb_DisplayWindow_setTitle(VALUE self, VALUE title)
{
	ensureWindow();
	g_window->setTitle(StringValueCStr(title));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_TITLE, title);
	return self;
}

VALUE rb_DisplayWindow_setIcon(VALUE self, VALUE arg)
{
	(void)self;
	ensureWindow();
	// LiteRGSS2 accepted either a file path or an Image instance (PSDK's
	// 0 Dependencies/3 Graphics module Improvements/00001 Window Icon.rb
	// passes an Image). Branch on the Ruby type.
	if (rb_obj_is_kind_of(arg, rb_cImage) == Qtrue) {
		auto *img = get_image(arg);
		if (!img->valid()) return self;
		g_window->setIcon(img->image);
		return self;
	}
	rb_check_type(arg, T_STRING);
	const char* p = StringValueCStr(arg);
	// LiteRGSS2 was silent on missing files — match that.
	if (!std::filesystem::exists(p)) return self;
	auto img = cgss::Image::create(p);
	if (img.width() == 0 || img.height() == 0) return self;
	g_window->setIcon(img);
	return self;
}

VALUE rb_DisplayWindow_setVsync(VALUE self, VALUE enabled)
{
	ensureWindow();
	g_window->setVsync(RTEST(enabled));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_VSYNC, RTEST(enabled) ? Qtrue : Qfalse);
	return self;
}

VALUE rb_DisplayWindow_setFps(VALUE self, VALUE fps)
{
	ensureWindow();
	g_window->setFrameRate(NUM2UINT(fps));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_FRAMERATE, fps);
	return self;
}

VALUE rb_DisplayWindow_getX(VALUE self)
{
	(void)self;
	ensureWindow();
	return INT2NUM(g_window->getX());
}

VALUE rb_DisplayWindow_getY(VALUE self)
{
	(void)self;
	ensureWindow();
	return INT2NUM(g_window->getY());
}

VALUE rb_DisplayWindow_move(VALUE self, VALUE x, VALUE y)
{
	(void)self;
	ensureWindow();
	g_window->move(NUM2INT(x), NUM2INT(y));
	return self;
}

// LiteRGSS2 exposed desktop dimensions as instance methods AND as class
// methods. Provide both so PSDK's `LiteRGSS::DisplayWindow.desktop_width`
// works pre-instance.
VALUE rb_DisplayWindow_desktopWidth(VALUE self)
{
	(void)self;
	return UINT2NUM(cgss::DisplayWindow::DesktopWidth());
}

VALUE rb_DisplayWindow_desktopHeight(VALUE self)
{
	(void)self;
	return UINT2NUM(cgss::DisplayWindow::DesktopHeight());
}

// `update` runs the atomic clear → drawables → postProcessing → display
// pipeline via cgss::DisplayWindow::draw(), then drains pending events into
// the registered on_* callbacks. Matches LiteRGSS2's update semantics where
// a single call advances both render and input.
VALUE rb_DisplayWindow_update(VALUE self)
{
	ensureWindow();
	g_window->draw();
	if (RTEST(rb_ivar_get(self, iv_polling))) drain_events_internal(self);
	return g_window->isOpen() ? Qtrue : Qfalse;
}

// LiteRGSS2: update with no input pump. Used during transitions where
// PSDK doesn't want callbacks to fire.
VALUE rb_DisplayWindow_updateNoInput(VALUE self)
{
	(void)self;
	ensureWindow();
	g_window->draw();
	return self;
}

// LiteRGSS2: drain events without rendering.
VALUE rb_DisplayWindow_updateOnlyInput(VALUE self)
{
	ensureWindow();
	drain_events_internal(self);
	return self;
}

// Backwards compat: the pre-Phase-1 lifecycle had a separate `present` call.
// cgss::DisplayWindow::draw() now handles clear+display atomically, so this
// is a no-op kept bound for callers that still issue it.
VALUE rb_DisplayWindow_present(VALUE self)
{
	return self;
}

VALUE rb_DisplayWindow_close(VALUE self)
{
	if (g_window != nullptr) {
		g_window->stop();
		g_window.reset();
	}
	rb_ivar_set(self, iv_disposed, Qtrue);
	g_display_window_instance = Qnil;
	return self;
}

VALUE rb_DisplayWindow_disposed(VALUE self)
{
	return RTEST(rb_ivar_get(self, iv_disposed)) ? Qtrue : Qfalse;
}

VALUE rb_DisplayWindow_should_close(VALUE self)
{
	(void)self;
	if (g_window == nullptr) return Qtrue;
	return g_window->isOpen() ? Qfalse : Qtrue;
}

// --- LiteRGSS2 state knobs -------------------------------------------------

VALUE rb_DisplayWindow_getBrightness(VALUE self) { return rb_ivar_get(self, iv_brightness); }
VALUE rb_DisplayWindow_setBrightness(VALUE self, VALUE val)
{
	long b = NUM2LONG(val);
	if (b < 0)   b = 0;
	if (b > 255) b = 255;
	rb_ivar_set(self, iv_brightness, INT2NUM(static_cast<int>(b)));
	if (g_window != nullptr) g_window->setBrightness(static_cast<unsigned char>(b));
	return val;
}

VALUE rb_DisplayWindow_getShader(VALUE self) { return rb_ivar_get(self, iv_shader); }
VALUE rb_DisplayWindow_setShader(VALUE self, VALUE val)
{
	if (!NIL_P(val) && rb_obj_is_kind_of(val, rb_cBlendMode) != Qtrue) {
		rb_raise(rb_eRGSSError, "DisplayWindow shader must be a BlendMode (or subclass).");
	}
	rb_ivar_set(self, iv_shader, val);
	if (g_window != nullptr) {
		if (NIL_P(val)) {
			g_window->bindRenderStates(nullptr);
		} else {
			g_window->bindRenderStates(rb::GetPtr<RenderStatesElement>(val));
		}
	}
	return val;
}

VALUE rb_DisplayWindow_setPolling(VALUE self, VALUE val)
{
	rb_ivar_set(self, iv_polling, RTEST(val) ? Qtrue : Qfalse);
	return val;
}

// `sort_z` forwards to the cgss::DisplayWindow's View::sortZ — actually
// reorders the DrawableStack now that drawables register through Phases 2-4.
VALUE rb_DisplayWindow_sortZ(VALUE self)
{
	if (g_window != nullptr) g_window->sortZ();
	return self;
}

VALUE rb_DisplayWindow_snapToBitmap(VALUE self)
{
	(void)self;
	if (g_window == nullptr) {
		VALUE args[2] = { INT2NUM(1), INT2NUM(1) };
		return rb_class_new_instance(2, args, rb_cImage);
	}
	auto snapshot = g_window->takeSnapshot();
	if (!snapshot) {
		VALUE arr = rb_ivar_get(self, iv_settings);
		const long w = NUM2LONG(rb_ary_entry(arr, SI_WIDTH));
		const long h = NUM2LONG(rb_ary_entry(arr, SI_HEIGHT));
		VALUE args[2] = { LONG2NUM(w > 0 ? w : 1), LONG2NUM(h > 0 ? h : 1) };
		return rb_class_new_instance(2, args, rb_cImage);
	}
	const auto sz = snapshot->getSize();
	VALUE args[2] = { UINT2NUM(sz.x), UINT2NUM(sz.y) };
	return rb_class_new_instance(2, args, rb_cImage);
}

// --- Event callbacks -------------------------------------------------------

// Setter generator: stash the proc keyed by Ruby Symbol on the per-instance
// callback hash. Dispatch reads back via rb_hash_lookup during drain.
#define DEFINE_CB_SETTER(rb_name, c_name)                                  \
	static VALUE rb_DisplayWindow_set_##c_name(VALUE self, VALUE cb)       \
	{                                                                       \
		VALUE h = rb_ivar_get(self, iv_event_callbacks);                    \
		rb_hash_aset(h, ID2SYM(rb_intern(rb_name)), cb);                    \
		return cb;                                                          \
	}

DEFINE_CB_SETTER("on_closed",                  on_closed)
DEFINE_CB_SETTER("on_lost_focus",              on_lost_focus)
DEFINE_CB_SETTER("on_gained_focus",            on_gained_focus)
DEFINE_CB_SETTER("on_text_entered",            on_text_entered)
DEFINE_CB_SETTER("on_key_pressed",             on_key_pressed)
DEFINE_CB_SETTER("on_key_released",            on_key_released)
DEFINE_CB_SETTER("on_mouse_wheel_scrolled",    on_mouse_wheel_scrolled)
DEFINE_CB_SETTER("on_mouse_button_pressed",    on_mouse_button_pressed)
DEFINE_CB_SETTER("on_mouse_button_released",   on_mouse_button_released)
DEFINE_CB_SETTER("on_mouse_moved",             on_mouse_moved)
DEFINE_CB_SETTER("on_mouse_entered",           on_mouse_entered)
DEFINE_CB_SETTER("on_mouse_left",              on_mouse_left)
DEFINE_CB_SETTER("on_joystick_button_pressed", on_joystick_button_pressed)
DEFINE_CB_SETTER("on_joystick_button_released",on_joystick_button_released)
DEFINE_CB_SETTER("on_joystick_connected",      on_joystick_connected)
DEFINE_CB_SETTER("on_joystick_disconnected",   on_joystick_disconnected)
DEFINE_CB_SETTER("on_joystick_moved",          on_joystick_moved)
DEFINE_CB_SETTER("on_touch_began",             on_touch_began)
DEFINE_CB_SETTER("on_touch_moved",             on_touch_moved)
DEFINE_CB_SETTER("on_touch_ended",             on_touch_ended)
DEFINE_CB_SETTER("on_sensor_changed",          on_sensor_changed)

// Helper: invoke a stored callback by Symbol key with a single arg or
// variadic argv. Silently no-ops when the slot is unset (rb_hash_lookup
// returns Qnil → rb_proc_call_with_block on Qnil raises; guard).
static void invoke_cb(VALUE cb_hash, const char* sym_name, int argc, const VALUE* argv)
{
	VALUE cb = rb_hash_lookup(cb_hash, ID2SYM(rb_intern(sym_name)));
	if (NIL_P(cb)) return;
	rb_proc_call_with_block(cb, argc, argv, Qnil);
}

// Translates raw cgss events into LiteRGSS2-shape callbacks. This is the
// heart of the C-side replacement for LiteRGSS_compat#drain_events.
static VALUE drain_events_internal(VALUE self)
{
	VALUE cb_hash = rb_ivar_get(self, iv_event_callbacks);
	cgss::Event ev;
	while (g_window != nullptr && g_window->popEvent(ev))
	{
		using ET = cgss::EventType;
		switch (ev.type())
		{
			case ET::Closed: {
				// LiteRGSS2 surfaced a window-close request as a raised
				// `LiteRGSS::DisplayWindow::ClosedWindowError` so the main loop
				// would bail out of `Graphics.update`; PSDK's on_closed proc
				// returns truthy specifically to opt into that. Match that
				// behaviour: invoke the user's callback first, and if it
				// returns truthy (or is unset) raise the same error to break
				// the run loop. Without this, PSDK nils its @window and then
				// spins forever in update_no_input — that's the close-window
				// freeze. Emit the error AFTER tearing down the window so any
				// `ensure` block downstream still sees a disposed state.
				VALUE cb = rb_hash_lookup(cb_hash, ID2SYM(rb_intern("on_closed")));
				const bool do_raise = NIL_P(cb)
					|| RTEST(rb_proc_call_with_block(cb, 0, nullptr, Qnil));
				g_window->stop();
				rb_ivar_set(self, iv_disposed, Qtrue);
				if (do_raise) {
					rb_raise(rb_eClosedWindowError,
					         "Game Window has been closed by user");
				}
				break;
			}
			case ET::LostFocus: {
				rb_ivar_set(self, iv_focus, Qfalse);
				invoke_cb(cb_hash, "on_lost_focus", 0, nullptr);
				break;
			}
			case ET::GainedFocus: {
				rb_ivar_set(self, iv_focus, Qtrue);
				invoke_cb(cb_hash, "on_gained_focus", 0, nullptr);
				break;
			}
			case ET::TextEntered: {
				const auto p = ev.asTextEntered();
				if (!p) break;
				// LiteRGSS2 callback signature: (utf8_str, codepoint).
				char buf[8] = {};
				const uint32_t u = p->unicode;
				int len = 0;
				if (u < 0x80)        { buf[len++] = (char)u; }
				else if (u < 0x800)  { buf[len++] = (char)(0xC0 | (u >> 6));
				                        buf[len++] = (char)(0x80 | (u & 0x3F)); }
				else if (u < 0x10000){ buf[len++] = (char)(0xE0 | (u >> 12));
				                        buf[len++] = (char)(0x80 | ((u >> 6) & 0x3F));
				                        buf[len++] = (char)(0x80 | (u & 0x3F)); }
				else                 { buf[len++] = (char)(0xF0 | (u >> 18));
				                        buf[len++] = (char)(0x80 | ((u >> 12) & 0x3F));
				                        buf[len++] = (char)(0x80 | ((u >> 6) & 0x3F));
				                        buf[len++] = (char)(0x80 | (u & 0x3F)); }
				VALUE args[2] = { rb_utf8_str_new(buf, len), UINT2NUM(u) };
				invoke_cb(cb_hash, "on_text_entered", 2, args);
				break;
			}
			case ET::Resized: {
				const auto p = ev.asResize();
				if (!p) break;
				VALUE arr = rb_ivar_get(self, iv_settings);
				if (!NIL_P(arr)) {
					rb_ary_store(arr, SI_WIDTH,  UINT2NUM(p->width));
					rb_ary_store(arr, SI_HEIGHT, UINT2NUM(p->height));
				}
				break;
			}
			case ET::MouseMoved: {
				const auto p = ev.asMouseMove();
				if (!p) break;
				VALUE args[2] = { INT2NUM(p->x), INT2NUM(p->y) };
				invoke_cb(cb_hash, "on_mouse_moved", 2, args);
				break;
			}
			case ET::MouseEntered:
				invoke_cb(cb_hash, "on_mouse_entered", 0, nullptr);
				break;
			case ET::MouseLeft:
				invoke_cb(cb_hash, "on_mouse_left", 0, nullptr);
				break;
			case ET::MouseButtonPressed: {
				const auto p = ev.asMouseButtonPress();
				if (!p) break;
				g_last_mouse[static_cast<int>(p->button)] = true;
				VALUE args[1] = { INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_mouse_button_pressed", 1, args);
				break;
			}
			case ET::MouseButtonReleased: {
				const auto p = ev.asMouseButtonRelease();
				if (!p) break;
				g_last_mouse[static_cast<int>(p->button)] = false;
				VALUE args[1] = { INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_mouse_button_released", 1, args);
				break;
			}
			case ET::MouseWheelScrolled: {
				const auto p = ev.asMouseWheel();
				if (!p) break;
				// LiteRGSS2: (wheel_id, delta) where wheel_id is the mouse
				// constant (Vertical/HorizontalWheel). Read from
				// LiteRGSS::Inputs::Mouse to stay in sync with PSDK's lookup.
				VALUE rb_mInputs_local = rb_const_get(rb_mLiteRGSS, rb_intern("Inputs"));
				VALUE rb_mMouse = rb_const_get(rb_mInputs_local, rb_intern("Mouse"));
				VALUE wheel_const = rb_const_get(rb_mMouse,
					rb_intern(p->horizontal ? "HorizontalWheel" : "VerticalWheel"));
				VALUE args[2] = { wheel_const, DBL2NUM(static_cast<double>(p->delta)) };
				invoke_cb(cb_hash, "on_mouse_wheel_scrolled", 2, args);
				break;
			}
			case ET::KeyPressed: {
				const auto p = ev.asKeyPress();
				if (!p) break;
				const int code = static_cast<int>(p->code);
				g_last_keys[code] = true;
				// LiteRGSS2 callback signature: (code, scan, alt). LiteRGSS3
				// only carries one keycode — pass it as both.
				VALUE args[3] = {
					INT2NUM(code), INT2NUM(code),
					p->alt ? Qtrue : Qfalse,
				};
				invoke_cb(cb_hash, "on_key_pressed", 3, args);
				break;
			}
			case ET::KeyReleased: {
				const auto p = ev.asKeyRelease();
				if (!p) break;
				const int code = static_cast<int>(p->code);
				g_last_keys[code] = false;
				VALUE args[2] = { INT2NUM(code), INT2NUM(code) };
				invoke_cb(cb_hash, "on_key_released", 2, args);
				break;
			}
			case ET::JoystickButtonPressed: {
				const auto p = ev.asGamepadButtonPress();
				if (!p) break;
				VALUE args[2] = { UINT2NUM(p->id), INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_joystick_button_pressed", 2, args);
				break;
			}
			case ET::JoystickButtonReleased: {
				const auto p = ev.asGamepadButtonRelease();
				if (!p) break;
				VALUE args[2] = { UINT2NUM(p->id), INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_joystick_button_released", 2, args);
				break;
			}
			case ET::JoystickMoved: {
				const auto p = ev.asGamepadMove();
				if (!p) break;
				// LiteRGSS2 sent positions as integers in the [-100, 100] range.
				const long pos = static_cast<long>(p->position * 100.0f);
				VALUE args[3] = {
					UINT2NUM(p->id), INT2NUM(static_cast<int>(p->axis)),
					LONG2NUM(pos),
				};
				invoke_cb(cb_hash, "on_joystick_moved", 3, args);
				break;
			}
			case ET::JoystickConnected: {
				const auto p = ev.asGamepadConnect();
				if (!p) break;
				VALUE args[1] = { UINT2NUM(p->id) };
				invoke_cb(cb_hash, "on_joystick_connected", 1, args);
				break;
			}
			case ET::JoystickDisconnected: {
				const auto p = ev.asGamepadDisconnect();
				if (!p) break;
				VALUE args[1] = { UINT2NUM(p->id) };
				invoke_cb(cb_hash, "on_joystick_disconnected", 1, args);
				break;
			}
			case ET::TouchBegan: {
				const auto p = ev.asTouch();
				if (!p) break;
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x), INT2NUM(p->y) };
				invoke_cb(cb_hash, "on_touch_began", 3, args);
				break;
			}
			case ET::TouchMoved: {
				const auto p = ev.asTouch();
				if (!p) break;
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x), INT2NUM(p->y) };
				invoke_cb(cb_hash, "on_touch_moved", 3, args);
				break;
			}
			case ET::TouchEnded: {
				const auto p = ev.asTouch();
				if (!p) break;
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x), INT2NUM(p->y) };
				invoke_cb(cb_hash, "on_touch_ended", 3, args);
				break;
			}
			case ET::SensorChanged:
				// No backend currently emits this. Reserved for future use.
				break;
		}
	}
	return self;
}

// `poll_event` retains the LiteRGSS3 yield-typed-Struct API for callers
// that don't use the on_* callbacks. Coexists with the callback dispatch
// — it pumps the same queue but yields raw Structs rather than firing
// callbacks. Used by tests and any caller that wants direct access.
VALUE rb_DisplayWindow_poll_event(VALUE self)
{
	(void)self;
	ensureWindow();
	const bool has_block = rb_block_given_p();
	long count = 0;
	cgss::Event ev;
	while (g_window != nullptr && g_window->popEvent(ev))
	{
		++count;
		if (ev.type() == cgss::EventType::Closed) g_window->stop();
		if (has_block) {
			VALUE rb_ev = build_ruby_event(ev);
			if (rb_ev != Qnil) rb_yield(rb_ev);
		}
	}
	return has_block ? self : LONG2NUM(count);
}

// --- Class-method desktop_width/height ------------------------------------

static VALUE rb_DisplayWindow_class_desktopWidth(VALUE self)
{
	(void)self;
	return UINT2NUM(cgss::DisplayWindow::DesktopWidth());
}

static VALUE rb_DisplayWindow_class_desktopHeight(VALUE self)
{
	(void)self;
	return UINT2NUM(cgss::DisplayWindow::DesktopHeight());
}

// --- Init ------------------------------------------------------------------

void Init_DisplayWindow()
{
	rb_cDisplayWindow = rb_define_class_under(rb_mLiteRGSS, "DisplayWindow", rb_cObject);

	// LiteRGSS2's ClosedWindowError, raised when the main loop should
	// short-circuit out of update because the user closed the window.
	rb_eClosedWindowError = rb_define_class_under(rb_cDisplayWindow,
	                                              "ClosedWindowError",
	                                              rb_eRGSSError);

	iv_settings        = rb_intern("@settings");
	iv_brightness      = rb_intern("@brightness");
	iv_shader          = rb_intern("@shader");
	iv_polling         = rb_intern("@polling");
	iv_disposed        = rb_intern("@disposed");
	iv_focus           = rb_intern("@focus");
	iv_event_callbacks = rb_intern("@event_callbacks");

	rb_define_method(rb_cDisplayWindow, "initialize",       _rbf rb_DisplayWindow_initialize,        -1);
	rb_define_method(rb_cDisplayWindow, "open_window",      _rbf rb_DisplayWindow_open_window,       -1);
	rb_define_method(rb_cDisplayWindow, "settings",         _rbf rb_DisplayWindow_getSettings,        0);
	rb_define_method(rb_cDisplayWindow, "settings=",        _rbf rb_DisplayWindow_setSettings,        1);
	rb_define_method(rb_cDisplayWindow, "width",            _rbf rb_DisplayWindow_getWidth,           0);
	rb_define_method(rb_cDisplayWindow, "height",           _rbf rb_DisplayWindow_getHeight,          0);
	rb_define_method(rb_cDisplayWindow, "scale_window",     _rbf rb_DisplayWindow_scale,              1);
	rb_define_method(rb_cDisplayWindow, "resize_screen",    _rbf rb_DisplayWindow_resizeScreen,       2);
	rb_define_method(rb_cDisplayWindow, "set_title",        _rbf rb_DisplayWindow_setTitle,           1);
	rb_define_method(rb_cDisplayWindow, "set_icon",         _rbf rb_DisplayWindow_setIcon,            1);
	rb_define_method(rb_cDisplayWindow, "icon=",            _rbf rb_DisplayWindow_setIcon,            1);
	rb_define_method(rb_cDisplayWindow, "set_vsync",        _rbf rb_DisplayWindow_setVsync,           1);
	rb_define_method(rb_cDisplayWindow, "set_fps",          _rbf rb_DisplayWindow_setFps,             1);
	rb_define_method(rb_cDisplayWindow, "x",                _rbf rb_DisplayWindow_getX,               0);
	rb_define_method(rb_cDisplayWindow, "y",                _rbf rb_DisplayWindow_getY,               0);
	rb_define_method(rb_cDisplayWindow, "move",             _rbf rb_DisplayWindow_move,               2);
	rb_define_method(rb_cDisplayWindow, "desktop_width",    _rbf rb_DisplayWindow_desktopWidth,       0);
	rb_define_method(rb_cDisplayWindow, "desktop_height",   _rbf rb_DisplayWindow_desktopHeight,      0);
	rb_define_method(rb_cDisplayWindow, "update",           _rbf rb_DisplayWindow_update,             0);
	rb_define_method(rb_cDisplayWindow, "update_no_input",  _rbf rb_DisplayWindow_updateNoInput,      0);
	rb_define_method(rb_cDisplayWindow, "update_only_input",_rbf rb_DisplayWindow_updateOnlyInput,    0);
	rb_define_method(rb_cDisplayWindow, "present",          _rbf rb_DisplayWindow_present,            0);
	rb_define_method(rb_cDisplayWindow, "close_window",     _rbf rb_DisplayWindow_close,              0);
	rb_define_method(rb_cDisplayWindow, "dispose",          _rbf rb_DisplayWindow_close,              0);
	rb_define_method(rb_cDisplayWindow, "disposed?",        _rbf rb_DisplayWindow_disposed,           0);
	rb_define_method(rb_cDisplayWindow, "should_close?",    _rbf rb_DisplayWindow_should_close,       0);
	rb_define_method(rb_cDisplayWindow, "poll_event",       _rbf rb_DisplayWindow_poll_event,         0);

	rb_define_method(rb_cDisplayWindow, "brightness",       _rbf rb_DisplayWindow_getBrightness,      0);
	rb_define_method(rb_cDisplayWindow, "brightness=",      _rbf rb_DisplayWindow_setBrightness,      1);
	rb_define_method(rb_cDisplayWindow, "shader",           _rbf rb_DisplayWindow_getShader,          0);
	rb_define_method(rb_cDisplayWindow, "shader=",          _rbf rb_DisplayWindow_setShader,          1);
	rb_define_method(rb_cDisplayWindow, "polling=",         _rbf rb_DisplayWindow_setPolling,         1);
	rb_define_method(rb_cDisplayWindow, "sort_z",           _rbf rb_DisplayWindow_sortZ,              0);
	rb_define_method(rb_cDisplayWindow, "snap_to_bitmap",   _rbf rb_DisplayWindow_snapToBitmap,       0);

	// Class-method desktop dimensions for pre-instance reads.
	rb_define_singleton_method(rb_cDisplayWindow, "desktop_width",  _rbf rb_DisplayWindow_class_desktopWidth,  0);
	rb_define_singleton_method(rb_cDisplayWindow, "desktop_height", _rbf rb_DisplayWindow_class_desktopHeight, 0);

	// Event callback setters.
	rb_define_method(rb_cDisplayWindow, "on_closed=",                  _rbf rb_DisplayWindow_set_on_closed,                  1);
	rb_define_method(rb_cDisplayWindow, "on_lost_focus=",              _rbf rb_DisplayWindow_set_on_lost_focus,              1);
	rb_define_method(rb_cDisplayWindow, "on_gained_focus=",            _rbf rb_DisplayWindow_set_on_gained_focus,            1);
	rb_define_method(rb_cDisplayWindow, "on_text_entered=",            _rbf rb_DisplayWindow_set_on_text_entered,            1);
	rb_define_method(rb_cDisplayWindow, "on_key_pressed=",             _rbf rb_DisplayWindow_set_on_key_pressed,             1);
	rb_define_method(rb_cDisplayWindow, "on_key_released=",            _rbf rb_DisplayWindow_set_on_key_released,            1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_wheel_scrolled=",    _rbf rb_DisplayWindow_set_on_mouse_wheel_scrolled,    1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_button_pressed=",    _rbf rb_DisplayWindow_set_on_mouse_button_pressed,    1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_button_released=",   _rbf rb_DisplayWindow_set_on_mouse_button_released,   1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_moved=",             _rbf rb_DisplayWindow_set_on_mouse_moved,             1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_entered=",           _rbf rb_DisplayWindow_set_on_mouse_entered,           1);
	rb_define_method(rb_cDisplayWindow, "on_mouse_left=",              _rbf rb_DisplayWindow_set_on_mouse_left,              1);
	rb_define_method(rb_cDisplayWindow, "on_joystick_button_pressed=", _rbf rb_DisplayWindow_set_on_joystick_button_pressed, 1);
	rb_define_method(rb_cDisplayWindow, "on_joystick_button_released=",_rbf rb_DisplayWindow_set_on_joystick_button_released,1);
	rb_define_method(rb_cDisplayWindow, "on_joystick_connected=",      _rbf rb_DisplayWindow_set_on_joystick_connected,      1);
	rb_define_method(rb_cDisplayWindow, "on_joystick_disconnected=",   _rbf rb_DisplayWindow_set_on_joystick_disconnected,   1);
	rb_define_method(rb_cDisplayWindow, "on_joystick_moved=",          _rbf rb_DisplayWindow_set_on_joystick_moved,          1);
	rb_define_method(rb_cDisplayWindow, "on_touch_began=",             _rbf rb_DisplayWindow_set_on_touch_began,             1);
	rb_define_method(rb_cDisplayWindow, "on_touch_moved=",             _rbf rb_DisplayWindow_set_on_touch_moved,             1);
	rb_define_method(rb_cDisplayWindow, "on_touch_ended=",             _rbf rb_DisplayWindow_set_on_touch_ended,             1);
	rb_define_method(rb_cDisplayWindow, "on_sensor_changed=",          _rbf rb_DisplayWindow_set_on_sensor_changed,          1);
}
