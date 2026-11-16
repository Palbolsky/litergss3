#include "DisplayWindow.h"

#include <ruby/thread.h>

#include <cstdlib>
#include <cstring>

#include "LiteRGSS.h"
#include "../events/Events.h"
#include "../inputs/SfJoystick.h"
#include "../inputs/SfMouse.h"
#include "../graphics/BlendMode.h"
#include "../graphics/Image.h"
#include "../rbAdapter.h"

#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Events/Event.h"
#include "LiteCGSS/Image/Image.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/DisplayWindow.h"

#include <filesystem>
#include <memory>
#include <string>
#include <algorithm>
#include <vector>

// Each Ruby DisplayWindow owns its cgss::DisplayWindow (the native window
// plus the View-side DrawableStack that Sprite/Viewport/Text/Shape register
// into), as in LiteRGSS2: a game can open several (PSDK opens its error
// screen as a second window, from a thread of its own). Backends with one OS
// window (raylib) stack them: the last opened owns the screen.
namespace {
	struct DisplayWindowData {
		std::unique_ptr<cgss::DisplayWindow> window;
		// The game's place on a window larger than it (game_area=): 0, the
		// game is the whole window.
		long gameWidth = 0;
		long gameHeight = 0;
		// A thread calling update while another one is inside it (its frame
		// drawn without the GVL) returns at once, as in LiteRGSS2.
		bool insideUpdate = false;
	};

	// Open windows, oldest first: drawables created without a DisplayWindow
	// go to the oldest (the game's).
	std::vector<DisplayWindowData*> g_open_windows;

	void forget_window(DisplayWindowData* d)
	{
		g_open_windows.erase(std::remove(g_open_windows.begin(), g_open_windows.end(), d), g_open_windows.end());
	}

	// Ends the window (its GL resources need the context: the thread holds it).
	void close_window(DisplayWindowData* d)
	{
		if (d->window == nullptr) return;
		forget_window(d);
		d->window->stop();
		d->window.reset();
	}

	void dw_free(void* ptr)
	{
		auto* d = static_cast<DisplayWindowData*>(ptr);
		close_window(d);
		delete d;
	}

	size_t dw_memsize(const void*) { return sizeof(DisplayWindowData); }

	const rb_data_type_t kDisplayWindowType = {
		"LiteRGSS::DisplayWindow",
		{ nullptr, dw_free, dw_memsize, nullptr, { nullptr } },
		nullptr, nullptr, 0
	};

	VALUE dw_alloc(VALUE klass)
	{
		return TypedData_Wrap_Struct(klass, &kDisplayWindowType, new DisplayWindowData{});
	}

	DisplayWindowData& data_of(VALUE self)
	{
		return *static_cast<DisplayWindowData*>(rb_check_typeddata(self, &kDisplayWindowType));
	}

	cgss::DisplayWindow* window_ptr(VALUE self)
	{
		return data_of(self).window.get();
	}

	cgss::DisplayWindow& window_of(VALUE self)
	{
		auto* window = window_ptr(self);
		if (window == nullptr) {
			rb_raise(rb_eRGSSError, "Window not opened — call DisplayWindow.new first");
		}
		return *window;
	}

	// Backends with one GL context for all windows (raylib): it follows the
	// Ruby thread holding the GVL, so a window driven from another thread
	// (PSDK's error screen) finds it current. Released when a thread lets the
	// GVL go, acquired when one gets it back; a frame drawn without the GVL
	// holds it meanwhile (draw_without_gvl).
	void context_follows_gvl(rb_event_flag_t event, const rb_internal_thread_event_data_t*, void*)
	{
		if (event == RUBY_INTERNAL_THREAD_EVENT_RESUMED) {
			cgss::DisplayWindow::AcquireContext();
		} else {
			cgss::DisplayWindow::ReleaseContext();
		}
	}

	void install_context_hook()
	{
		static bool installed = false;
		if (installed || !cgss::DisplayWindow::ContextFollowsThread()) return;
		installed = true;
		// This thread runs Ruby, so it has the GVL: it takes the context first.
		cgss::DisplayWindow::AcquireContext();
		rb_internal_thread_add_event_hook(context_follows_gvl,
			RUBY_INTERNAL_THREAD_EVENT_RESUMED | RUBY_INTERNAL_THREAD_EVENT_SUSPENDED | RUBY_INTERNAL_THREAD_EVENT_EXITED,
			nullptr);
	}
}

VALUE rb_cDisplayWindow      = Qnil;
VALUE rb_eClosedWindowError  = Qnil;
VALUE rb_eStoppedWindowError = Qnil;

cgss::DisplayWindow* get_active_display_window()
{
	return g_open_windows.empty() ? nullptr : g_open_windows.front()->window.get();
}

cgss::DisplayWindow* display_window_for(VALUE parent)
{
	if (RTEST(parent) && rb_obj_is_kind_of(parent, rb_cDisplayWindow) == Qtrue) {
		return window_ptr(parent);
	}
	return get_active_display_window();
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

// The title the window shows: the script's, plus LITERGSS_TITLE_SUFFIX if
// set (tools/psdk-demo.sh names the build there). The settings keep the
// script's own title.
static std::string window_title(const char* title)
{
	const char* suffix = std::getenv("LITERGSS_TITLE_SUFFIX");
	return suffix == nullptr ? std::string{title} : std::string{title} + suffix;
}

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

	settings.title        = window_title(NIL_P(title) ? "LiteRGSS" : StringValueCStr(title));
	settings.video.width  = NUM2UINT(width);
	settings.video.height = NUM2UINT(height);
	settings.video.scale  = NUM2DBL(scale);
	settings.video.bitsPerPixel = NUM2UINT(bpp);
	settings.frameRate    = NUM2UINT(framerate);
	settings.vSync        = RTEST(vsync);
	settings.fullscreen   = RTEST(fullscreen);
	settings.visibleMouse = RTEST(visible_mouse);
	// Never shown (still a real window and GL context): the test suite sets
	// it, so running the tests doesn't open windows on the desktop.
	const char* hidden = std::getenv("LITERGSS_HIDDEN_WINDOW");
	settings.hidden = hidden != nullptr && *hidden != '\0' && std::strcmp(hidden, "0") != 0;
	return settings;
}

static void open_window_with_settings(VALUE self, VALUE arr)
{
	auto settings = build_settings_from_array(arr);
	auto& d = data_of(self);
	if (d.window == nullptr) {
		install_context_hook();
		d.window = std::make_unique<cgss::DisplayWindow>(settings);
		g_open_windows.push_back(&d);
	} else {
		d.window->reload(std::move(settings), true);
	}
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

	open_window_with_settings(self, arr);
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

	open_window_with_settings(self, arr);
	rb_ivar_set(self, iv_disposed, Qfalse);
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
	auto* window = window_ptr(self);
	if (window == nullptr) return;
	// reload(..., false) defers the reload to the next draw() — exactly the
	// behaviour we want when settings change mid-loop.
	window->reload(build_settings_from_array(arr), false);
}

VALUE rb_DisplayWindow_setSettings(VALUE self, VALUE arr)
{
	apply_settings_array(self, arr);
	return arr;
}

// The game's place on a window larger than it (game_area=): the engine moves
// the game there (setOrigin), width / height answer the game's size and the
// mouse / touch positions reach the callbacks moved the same way.
namespace {
	bool hasGameArea(VALUE self) { return data_of(self).gameWidth > 0; }

	// Mouse and touch positions come in the window's pixels.
	cgss::IntPoint inputOrigin(VALUE self)
	{
		auto* window = window_ptr(self);
		return window != nullptr ? window->originPixels() : cgss::IntPoint{};
	}
}

// The whole window's width, black bars included
VALUE rb_DisplayWindow_screenWidth(VALUE self)
{
	return rb_ary_entry(rb_ivar_get(self, iv_settings), SI_WIDTH);
}

// The whole window's height, black bars included
VALUE rb_DisplayWindow_screenHeight(VALUE self)
{
	return rb_ary_entry(rb_ivar_get(self, iv_settings), SI_HEIGHT);
}

// The game's width: the whole window's without a game area
VALUE rb_DisplayWindow_getWidth(VALUE self)
{
	return hasGameArea(self) ? LONG2NUM(data_of(self).gameWidth) : rb_DisplayWindow_screenWidth(self);
}

// The game's height: the whole window's without a game area
VALUE rb_DisplayWindow_getHeight(VALUE self)
{
	return hasGameArea(self) ? LONG2NUM(data_of(self).gameHeight) : rb_DisplayWindow_screenHeight(self);
}

// [x, y, width, height] in window pixels, nil when the game is the whole window
VALUE rb_DisplayWindow_getGameArea(VALUE self)
{
	auto* window = window_ptr(self);
	if (!hasGameArea(self) || window == nullptr) return Qnil;
	const auto& d = data_of(self);
	return rb_ary_new_from_args(4, LONG2NUM(window->originX()), LONG2NUM(window->originY()),
		LONG2NUM(d.gameWidth), LONG2NUM(d.gameHeight));
}

// Put the game at its place on a window larger than it (nil: the whole window)
VALUE rb_DisplayWindow_setGameArea(VALUE self, VALUE area)
{
	auto& win = window_of(self);
	auto& d = data_of(self);
	if (NIL_P(area)) {
		d.gameWidth = d.gameHeight = 0;
		win.setOrigin(0, 0);
		return area;
	}
	Check_Type(area, T_ARRAY);
	if (RARRAY_LEN(area) != 4) {
		rb_raise(rb_eArgError, "Expected [x, y, width, height] as the game area, or nil");
	}
	const long x = NUM2LONG(rb_ary_entry(area, 0));
	const long y = NUM2LONG(rb_ary_entry(area, 1));
	const long width = NUM2LONG(rb_ary_entry(area, 2));
	const long height = NUM2LONG(rb_ary_entry(area, 3));
	const long sw = NUM2LONG(rb_DisplayWindow_screenWidth(self));
	const long sh = NUM2LONG(rb_DisplayWindow_screenHeight(self));
	if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > sw || y + height > sh) {
		rb_raise(rb_eArgError, "The game area [%ld, %ld, %ld, %ld] is not inside the %ldx%ld window",
			x, y, width, height, sw, sh);
	}
	d.gameWidth = width;
	d.gameHeight = height;
	win.setOrigin(x, y);
	return area;
}

// --- Window operations -----------------------------------------------------

VALUE rb_DisplayWindow_scale(VALUE self, VALUE scale)
{
	auto& win = window_of(self);
	win.setScale(NUM2DBL(scale));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_SCALE, scale);
	return self;
}

VALUE rb_DisplayWindow_resizeScreen(VALUE self, VALUE width, VALUE height)
{
	auto& win = window_of(self);
	win.resizeScreen(NUM2UINT(width), NUM2UINT(height));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) {
		rb_ary_store(arr, SI_WIDTH, width);
		rb_ary_store(arr, SI_HEIGHT, height);
	}
	return self;
}

VALUE rb_DisplayWindow_setTitle(VALUE self, VALUE title)
{
	auto& win = window_of(self);
	win.setTitle(window_title(StringValueCStr(title)).c_str());
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_TITLE, title);
	return self;
}

VALUE rb_DisplayWindow_setIcon(VALUE self, VALUE arg)
{
	(void)self;
	auto& win = window_of(self);
	// LiteRGSS2 accepted either a file path or an Image instance (PSDK's
	// 0 Dependencies/3 Graphics module Improvements/00001 Window Icon.rb
	// passes an Image). Branch on the Ruby type.
	if (rb_obj_is_kind_of(arg, rb_cImage) == Qtrue) {
		auto *img = get_image(arg);
		if (!img->valid()) return self;
		win.setIcon(img->image);
		return self;
	}
	rb_check_type(arg, T_STRING);
	const char* p = StringValueCStr(arg);
	// LiteRGSS2 was silent on missing files — match that.
	if (!std::filesystem::exists(p)) return self;
	auto img = cgss::Image::create(p);
	if (img.width() == 0 || img.height() == 0) return self;
	win.setIcon(img);
	return self;
}

VALUE rb_DisplayWindow_setVsync(VALUE self, VALUE enabled)
{
	auto& win = window_of(self);
	win.setVsync(RTEST(enabled));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_VSYNC, RTEST(enabled) ? Qtrue : Qfalse);
	return self;
}

VALUE rb_DisplayWindow_setFps(VALUE self, VALUE fps)
{
	auto& win = window_of(self);
	win.setFrameRate(NUM2UINT(fps));
	VALUE arr = rb_ivar_get(self, iv_settings);
	if (!NIL_P(arr)) rb_ary_store(arr, SI_FRAMERATE, fps);
	return self;
}

VALUE rb_DisplayWindow_getX(VALUE self)
{
	(void)self;
	auto& win = window_of(self);
	return INT2NUM(win.getX());
}

VALUE rb_DisplayWindow_getY(VALUE self)
{
	(void)self;
	auto& win = window_of(self);
	return INT2NUM(win.getY());
}

VALUE rb_DisplayWindow_move(VALUE self, VALUE x, VALUE y)
{
	(void)self;
	auto& win = window_of(self);
	win.move(NUM2INT(x), NUM2INT(y));
	return self;
}

VALUE rb_DisplayWindow_setX(VALUE self, VALUE x)
{
	auto& win = window_of(self);
	win.move(NUM2INT(x), win.getY());
	return self;
}

VALUE rb_DisplayWindow_setY(VALUE self, VALUE y)
{
	auto& win = window_of(self);
	win.move(win.getX(), NUM2INT(y));
	return self;
}

// [major, minor] of the GL context the window renders with.
VALUE rb_DisplayWindow_openGLVersion(VALUE self)
{
	(void)self;
	auto& win = window_of(self);
	const auto settings = win.getContextSettings();
	VALUE result = rb_ary_new_capa(2);
	rb_ary_push(result, UINT2NUM(settings.majorVersion));
	rb_ary_push(result, UINT2NUM(settings.minorVersion));
	return result;
}

// [[width, height], ...] of the 32 bpp fullscreen modes, best first.
VALUE rb_DisplayWindow_listResolutions(VALUE self)
{
	(void)self;
	VALUE result = rb_ary_new();
	for (const auto& mode : cgss::DisplayWindow::FullscreenModes()) {
		if (mode.bitsPerPixel != 32) continue;
		rb_ary_push(result, rb_ary_new_from_args(2, UINT2NUM(mode.width), UINT2NUM(mode.height)));
	}
	return result;
}

VALUE rb_DisplayWindow_maxTextureSize(VALUE self)
{
	(void)self;
	return UINT2NUM(cgss::Texture::getMaximumSize());
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
// As in LiteRGSS2, a frame is drawn outside the GVL: other Ruby threads
// run meanwhile, and a hosted Android game paused in the background (the
// frame waits for its surface) doesn't freeze them. No Ruby API in there.
namespace {
	void* draw_without_gvl(void* window)
	{
		// The GL context left with the GVL (context_follows_gvl): this frame
		// holds it until done.
		cgss::DisplayWindow::AcquireContext();
		static_cast<cgss::DisplayWindow*>(window)->draw();
		cgss::DisplayWindow::ReleaseContext();
		return nullptr;
	}

	void draw_frame(VALUE self)
	{
		// Bitmaps edited since the last frame reach the GPU now, while the
		// GVL (and the GL context) is still held.
		image_flush_pending_uploads();
		rb_thread_call_without_gvl(draw_without_gvl, &window_of(self), nullptr, nullptr);
	}

	VALUE leave_update(VALUE self)
	{
		data_of(self).insideUpdate = false;
		return Qnil;
	}

	// Runs body(self) as the window's one update in progress.
	VALUE guarded_update(VALUE (*body)(VALUE), VALUE self)
	{
		auto& d = data_of(self);
		if (d.insideUpdate) return Qnil;
		d.insideUpdate = true;
		return rb_ensure(body, self, leave_update, self);
	}

	VALUE update_body(VALUE self)
	{
		// Like LiteRGSS2: a window closed by anything but its Closed event
		// (which raises ClosedWindowError) stops the game loop with StoppedError.
		if (!window_of(self).isOpen()) {
			rb_raise(rb_eStoppedWindowError, "Game Window was closed during DisplayWindow.update by an unknown cause...");
		}
		draw_frame(self);
		if (RTEST(rb_ivar_get(self, iv_polling))) drain_events_internal(self);
		auto* window = window_ptr(self);
		return window != nullptr && window->isOpen() ? Qtrue : Qfalse;
	}

	VALUE update_no_input_body(VALUE self)
	{
		draw_frame(self);
		return self;
	}

	VALUE update_only_input_body(VALUE self)
	{
		drain_events_internal(self);
		return self;
	}
}

VALUE rb_DisplayWindow_update(VALUE self)
{
	auto& win = window_of(self);
	return guarded_update(update_body, self);
}

// LiteRGSS2: update with no input pump. Used during transitions where
// PSDK doesn't want callbacks to fire.
VALUE rb_DisplayWindow_updateNoInput(VALUE self)
{
	auto& win = window_of(self);
	guarded_update(update_no_input_body, self);
	return self;
}

// LiteRGSS2: drain events without rendering.
VALUE rb_DisplayWindow_updateOnlyInput(VALUE self)
{
	auto& win = window_of(self);
	guarded_update(update_only_input_body, self);
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
	close_window(&data_of(self));
	rb_ivar_set(self, iv_disposed, Qtrue);
	return self;
}

VALUE rb_DisplayWindow_disposed(VALUE self)
{
	return RTEST(rb_ivar_get(self, iv_disposed)) ? Qtrue : Qfalse;
}

VALUE rb_DisplayWindow_should_close(VALUE self)
{
	auto* window = window_ptr(self);
	if (window == nullptr) return Qtrue;
	return window->isOpen() ? Qfalse : Qtrue;
}

// --- LiteRGSS2 state knobs -------------------------------------------------

VALUE rb_DisplayWindow_getBrightness(VALUE self) { return rb_ivar_get(self, iv_brightness); }
VALUE rb_DisplayWindow_setBrightness(VALUE self, VALUE val)
{
	long b = NUM2LONG(val);
	if (b < 0)   b = 0;
	if (b > 255) b = 255;
	rb_ivar_set(self, iv_brightness, INT2NUM(static_cast<int>(b)));
	if (auto* window = window_ptr(self)) window->setBrightness(static_cast<unsigned char>(b));
	return val;
}

VALUE rb_DisplayWindow_getShader(VALUE self) { return rb_ivar_get(self, iv_shader); }
VALUE rb_DisplayWindow_setShader(VALUE self, VALUE val)
{
	if (!NIL_P(val) && rb_obj_is_kind_of(val, rb_cBlendMode) != Qtrue) {
		rb_raise(rb_eRGSSError, "DisplayWindow shader must be a BlendMode (or subclass).");
	}
	rb_ivar_set(self, iv_shader, val);
	if (auto* window = window_ptr(self)) {
		if (NIL_P(val)) {
			window->bindRenderStates(nullptr);
		} else {
			window->bindRenderStates(rb::GetPtr<RenderStatesElement>(val));
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
	if (auto* window = window_ptr(self)) window->sortZ();
	return self;
}

VALUE rb_DisplayWindow_snapToBitmap(VALUE self)
{
	auto* window = window_ptr(self);
	if (window == nullptr) {
		VALUE args[2] = { INT2NUM(1), INT2NUM(1) };
		return rb_class_new_instance(2, args, rb_cImage);
	}
	auto snapshot = window->takeSnapshot();
	if (!snapshot) {
		VALUE arr = rb_ivar_get(self, iv_settings);
		const long w = NUM2LONG(rb_ary_entry(arr, SI_WIDTH));
		const long h = NUM2LONG(rb_ary_entry(arr, SI_HEIGHT));
		VALUE args[2] = { LONG2NUM(w > 0 ? w : 1), LONG2NUM(h > 0 ? h : 1) };
		return rb_class_new_instance(2, args, rb_cImage);
	}
	return image_from_texture(std::move(*snapshot));
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
DEFINE_CB_SETTER("on_resized",                 on_resized)
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
	while (window_ptr(self) != nullptr && window_ptr(self)->popEvent(ev))
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
				// Like LiteRGSS2, only an explicit `false` keeps the window
				// open (nil included, any other answer closes it).
				VALUE cb = rb_hash_lookup(cb_hash, ID2SYM(rb_intern("on_closed")));
				const bool close = NIL_P(cb)
					|| rb_proc_call_with_block(cb, 0, nullptr, Qnil) != Qfalse;
				if (!close) break; // on_closed returned false: the window stays open
				window_of(self).stop();
				rb_ivar_set(self, iv_disposed, Qtrue);
				rb_raise(rb_eClosedWindowError, "Game Window has been closed by user");
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
				// The window's pixel size (scale included): the settings keep the
				// game's size, as in LiteRGSS2, which only forwards it.
				VALUE args[2] = { UINT2NUM(p->width), UINT2NUM(p->height) };
				invoke_cb(cb_hash, "on_resized", 2, args);
				break;
			}
			case ET::MouseMoved: {
				const auto p = ev.asMouseMove();
				if (!p) break;
				const auto origin = inputOrigin(self);
				VALUE args[2] = { INT2NUM(p->x - origin.x), INT2NUM(p->y - origin.y) };
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
				VALUE args[1] = { INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_mouse_button_pressed", 1, args);
				break;
			}
			case ET::MouseButtonReleased: {
				const auto p = ev.asMouseButtonRelease();
				if (!p) break;
				VALUE args[1] = { INT2NUM(static_cast<int>(p->button)) };
				invoke_cb(cb_hash, "on_mouse_button_released", 1, args);
				break;
			}
			case ET::MouseWheelScrolled: {
				const auto p = ev.asMouseWheel();
				if (!p) break;
				// LiteRGSS2: (wheel id, delta), the id being Sf::Mouse's.
				VALUE args[2] = {
					INT2NUM(p->horizontal ? sf_mouse::kHorizontalWheel : sf_mouse::kVerticalWheel),
					DBL2NUM(static_cast<double>(p->delta)),
				};
				invoke_cb(cb_hash, "on_mouse_wheel_scrolled", 2, args);
				break;
			}
			case ET::KeyPressed: {
				const auto p = ev.asKeyPress();
				if (!p) break;
				const int code = static_cast<int>(p->code);
				// LiteRGSS2 callback signature: (code, scancode, alt, control,
				// shift, system).
				VALUE args[6] = {
					INT2NUM(code), INT2NUM(static_cast<int>(p->scancode)),
					p->alt ? Qtrue : Qfalse, p->control ? Qtrue : Qfalse,
					p->shift ? Qtrue : Qfalse, p->system ? Qtrue : Qfalse,
				};
				invoke_cb(cb_hash, "on_key_pressed", 6, args);
				break;
			}
			case ET::KeyReleased: {
				const auto p = ev.asKeyRelease();
				if (!p) break;
				const int code = static_cast<int>(p->code);
				VALUE args[2] = { INT2NUM(code), INT2NUM(static_cast<int>(p->scancode)) };
				invoke_cb(cb_hash, "on_key_released", 2, args);
				break;
			}
			case ET::JoystickButtonPressed: {
				const auto p = ev.asGamepadButtonPress();
				if (!p) break;
				// LiteRGSS2 numbering (Sf::Joystick), see inputs/SfJoystick.h.
				const int index = legacy_joystick::buttonIndex(p->button);
				if (index < 0) break;
				VALUE args[2] = { UINT2NUM(p->id), INT2NUM(index) };
				invoke_cb(cb_hash, "on_joystick_button_pressed", 2, args);
				break;
			}
			case ET::JoystickButtonReleased: {
				const auto p = ev.asGamepadButtonRelease();
				if (!p) break;
				const int index = legacy_joystick::buttonIndex(p->button);
				if (index < 0) break;
				VALUE args[2] = { UINT2NUM(p->id), INT2NUM(index) };
				invoke_cb(cb_hash, "on_joystick_button_released", 2, args);
				break;
			}
			case ET::JoystickMoved: {
				const auto p = ev.asGamepadMove();
				if (!p) break;
				// LiteRGSS2: (id, Sf::Joystick axis id, Float in [-100, 100]).
				VALUE args[3] = {
					UINT2NUM(p->id), LONG2NUM(legacy_joystick::axisId(p->axis)),
					DBL2NUM(legacy_joystick::position(p->position)),
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
				const auto origin = inputOrigin(self);
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x - origin.x), INT2NUM(p->y - origin.y) };
				invoke_cb(cb_hash, "on_touch_began", 3, args);
				break;
			}
			case ET::TouchMoved: {
				const auto p = ev.asTouch();
				if (!p) break;
				const auto origin = inputOrigin(self);
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x - origin.x), INT2NUM(p->y - origin.y) };
				invoke_cb(cb_hash, "on_touch_moved", 3, args);
				break;
			}
			case ET::TouchEnded: {
				const auto p = ev.asTouch();
				if (!p) break;
				const auto origin = inputOrigin(self);
				VALUE args[3] = { UINT2NUM(p->finger), INT2NUM(p->x - origin.x), INT2NUM(p->y - origin.y) };
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
	auto& win = window_of(self);
	const bool has_block = rb_block_given_p();
	long count = 0;
	cgss::Event ev;
	while (window_ptr(self) != nullptr && win.popEvent(ev))
	{
		++count;
		if (ev.type() == cgss::EventType::Closed) win.stop();
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
	rb_define_alloc_func(rb_cDisplayWindow, dw_alloc);

	// LiteRGSS2's ClosedWindowError, raised when the main loop should
	// short-circuit out of update because the user closed the window.
	rb_eClosedWindowError = rb_define_class_under(rb_cDisplayWindow,
	                                              "ClosedWindowError",
	                                              rb_eRGSSError);
	rb_eStoppedWindowError = rb_define_class_under(rb_cDisplayWindow, "StoppedError", rb_eStandardError);

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
	rb_define_method(rb_cDisplayWindow, "screen_width",     _rbf rb_DisplayWindow_screenWidth,        0);
	rb_define_method(rb_cDisplayWindow, "screen_height",    _rbf rb_DisplayWindow_screenHeight,       0);
	rb_define_method(rb_cDisplayWindow, "game_area",        _rbf rb_DisplayWindow_getGameArea,        0);
	rb_define_method(rb_cDisplayWindow, "game_area=",       _rbf rb_DisplayWindow_setGameArea,        1);
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
	rb_define_method(rb_cDisplayWindow, "x=",               _rbf rb_DisplayWindow_setX,               1);
	rb_define_method(rb_cDisplayWindow, "y=",               _rbf rb_DisplayWindow_setY,               1);
	rb_define_method(rb_cDisplayWindow, "openGL_version",   _rbf rb_DisplayWindow_openGLVersion,      0);
	rb_define_method(rb_cDisplayWindow, "on_resized=",      _rbf rb_DisplayWindow_set_on_resized,     1);
	rb_define_singleton_method(rb_cDisplayWindow, "list_resolutions", _rbf rb_DisplayWindow_listResolutions, 0);
	rb_define_singleton_method(rb_cDisplayWindow, "max_texture_size", _rbf rb_DisplayWindow_maxTextureSize,  0);
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
