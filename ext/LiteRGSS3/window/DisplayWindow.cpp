#include "DisplayWindow.h"
#include "LiteRGSS.h"
#include "../events/Events.h"

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Events/Event.h"

#include <filesystem>
#include <memory>

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops     = Backend::Ops;

	// File-static window handle. Matches the singleton shape of the
	// previous raylib-direct implementation: LiteRGSS::Window is effectively
	// a static class (module-function style methods) and the underlying
	// native_window lives process-global. open_window allocates; close_window
	// resets.
	std::unique_ptr<Backend::native_window> g_window;
}

VALUE rb_cDisplayWindow = Qnil;

int base_width  = 640;
int base_height = 480;
double window_scale = 1.0;

cgss::backend::ActiveBackend::native_window* get_active_native_window()
{
	return g_window.get();
}

static void ensureWindow()
{
	// Window methods other than open_window require an instance. The
	// previous raylib-direct binding relied on raylib's global state being
	// no-op pre-InitWindow; under SFML calling methods on a null instance
	// would segfault, so guard explicitly.
	if (g_window == nullptr) {
		rb_raise(rb_eRGSSError, "Window not opened — call Window#open_window first");
	}
}

VALUE rb_DisplayWindow_open(int argc, VALUE* argv, VALUE self)
{
	VALUE width, height, title, scale;
	rb_scan_args(argc, argv, "04", &width, &height, &title, &scale);

	base_width  = RTEST(width)  ? NUM2INT(width)  : 640;
	base_height = RTEST(height) ? NUM2INT(height) : 480;
	window_scale = RTEST(scale) ? NUM2DBL(scale) : 1.0;

	const int w = static_cast<int>(base_width  * window_scale);
	const int h = static_cast<int>(base_height * window_scale);
	const char* t = RTEST(title) ? StringValueCStr(title) : "LiteRGSS";

	g_window = std::make_unique<Backend::native_window>();
	cgss::backend::WindowCreateOptions opts{};
	opts.width      = static_cast<unsigned int>(w);
	opts.height     = static_cast<unsigned int>(h);
	opts.fullscreen = false;
	Ops::window_create(*g_window, opts, t);
	return self;
}

VALUE rb_DisplayWindow_scale(VALUE self, VALUE scale)
{
	ensureWindow();
	window_scale = NUM2DBL(scale);
	const int w = static_cast<int>(base_width  * window_scale);
	const int h = static_cast<int>(base_height * window_scale);
	Ops::window_set_size(*g_window, static_cast<unsigned int>(w), static_cast<unsigned int>(h));
	return self;
}

VALUE rb_DisplayWindow_resizeScreen(VALUE self, VALUE width, VALUE height)
{
	ensureWindow();
	base_width  = NUM2INT(width);
	base_height = NUM2INT(height);
	Ops::window_set_size(*g_window,
	                     static_cast<unsigned int>(base_width),
	                     static_cast<unsigned int>(base_height));
	return self;
}

VALUE rb_DisplayWindow_setTitle(VALUE self, VALUE title)
{
	ensureWindow();
	Ops::window_set_title(*g_window, StringValueCStr(title));
	return self;
}

VALUE rb_DisplayWindow_setIcon(VALUE self, VALUE path)
{
	ensureWindow();
	const char* p = StringValueCStr(path);
	// Raylib's LoadImage returns an Image with data==nullptr on missing
	// files and our backend's window_set_icon_from_file silently no-ops in
	// that case. Guard here so callers get a Ruby-visible error.
	if (!std::filesystem::exists(p)) {
		rb_raise(rb_eRuntimeError, "Icon file not found: %s", p);
	}
	Ops::window_set_icon_from_file(*g_window, p);
	return self;
}

VALUE rb_DisplayWindow_setVsync(VALUE self, VALUE enabled)
{
	ensureWindow();
	Ops::window_set_vsync(*g_window, RTEST(enabled));
	return self;
}

VALUE rb_DisplayWindow_setFps(VALUE self, VALUE fps)
{
	ensureWindow();
	Ops::window_set_framerate_limit(*g_window, NUM2UINT(fps));
	return self;
}

VALUE rb_DisplayWindow_getX(VALUE self)
{
	(void)self;
	ensureWindow();
	return INT2NUM(Ops::window_get_position(*g_window).x);
}

VALUE rb_DisplayWindow_getY(VALUE self)
{
	(void)self;
	ensureWindow();
	return INT2NUM(Ops::window_get_position(*g_window).y);
}

VALUE rb_DisplayWindow_move(VALUE self, VALUE x, VALUE y)
{
	ensureWindow();
	Ops::window_set_position(*g_window, NUM2INT(x), NUM2INT(y));
	return self;
}

VALUE rb_DisplayWindow_desktopWidth(VALUE self)
{
	(void)self;
	return UINT2NUM(Ops::desktop_size().x);
}

VALUE rb_DisplayWindow_desktopHeight(VALUE self)
{
	(void)self;
	return UINT2NUM(Ops::desktop_size().y);
}

VALUE rb_DisplayWindow_update(VALUE self)
{
	(void)self;
	ensureWindow();
	Ops::window_clear(*g_window);
	return Ops::window_is_open(*g_window) ? Qtrue : Qfalse;
}

VALUE rb_DisplayWindow_present(VALUE self)
{
	ensureWindow();
	Ops::window_display(*g_window);
	return self;
}

VALUE rb_DisplayWindow_close(VALUE self)
{
	if (g_window != nullptr) {
		Ops::window_close(*g_window);
		g_window.reset();
	}
	return self;
}

VALUE rb_DisplayWindow_should_close(VALUE self)
{
	(void)self;
	if (g_window == nullptr) return Qtrue;
	// Under raylib, Ops::window_is_open checks WindowShouldClose directly —
	// works without an external event pump. Under SFML, `sf::Window::isOpen`
	// stays true until an explicit w.close() call; the user must pump
	// events via Window#poll_event so the Closed handler (see below) can
	// flip the state.
	return Ops::window_is_open(*g_window) ? Qfalse : Qtrue;
}

// Drain the event queue, yielding each typed Ruby event Struct to the block.
// With no block, consumes all events and returns the count as an Integer —
// lets callers pump the queue without Ruby-side allocation (e.g. to keep
// SFML responsive to OS events without caring about the payloads).
//
// Special-case: on Closed, close the window ourselves (SFML needs the
// explicit w.close() to flip isOpen; raylib's close latch is already set
// by the event synthesizer) so should_close? reflects it next frame.
VALUE rb_DisplayWindow_poll_event(VALUE self)
{
	ensureWindow();
	const bool has_block = rb_block_given_p();
	long count = 0;

	cgss::Event ev;
	// `cgss::backend::native_of(ev)` forwards through the Ops::native_of
	// static (Ops is friend of basic_Event — grants access to the private
	// native event handle). Same path basic_DisplayWindow::popEvent uses
	// internally.
	while (g_window != nullptr &&
	       Ops::window_poll_event(*g_window, cgss::backend::native_of(ev))) {
		++count;
		if (ev.type() == cgss::EventType::Closed) {
			Ops::window_close(*g_window);
		}
		if (has_block) {
			VALUE rb_ev = build_ruby_event(ev);
			if (rb_ev != Qnil) rb_yield(rb_ev);
		}
	}
	return has_block ? self : LONG2NUM(count);
}

void Init_DisplayWindow()
{
	rb_cDisplayWindow = rb_define_class_under(rb_mLiteRGSS, "DisplayWindow", rb_cObject);

	rb_define_method(rb_cDisplayWindow, "open_window",    _rbf rb_DisplayWindow_open,          -1);
	rb_define_method(rb_cDisplayWindow, "scale_window",   _rbf rb_DisplayWindow_scale,          1);
	rb_define_method(rb_cDisplayWindow, "resize_screen",  _rbf rb_DisplayWindow_resizeScreen,   2);
	rb_define_method(rb_cDisplayWindow, "set_title",      _rbf rb_DisplayWindow_setTitle,       1);
	rb_define_method(rb_cDisplayWindow, "set_icon",       _rbf rb_DisplayWindow_setIcon,        1);
	rb_define_method(rb_cDisplayWindow, "set_vsync",      _rbf rb_DisplayWindow_setVsync,       1);
	rb_define_method(rb_cDisplayWindow, "set_fps",        _rbf rb_DisplayWindow_setFps,         1);
	rb_define_method(rb_cDisplayWindow, "x",              _rbf rb_DisplayWindow_getX,           0);
	rb_define_method(rb_cDisplayWindow, "y",              _rbf rb_DisplayWindow_getY,           0);
	rb_define_method(rb_cDisplayWindow, "move",           _rbf rb_DisplayWindow_move,           2);
	rb_define_method(rb_cDisplayWindow, "desktop_width",  _rbf rb_DisplayWindow_desktopWidth,   0);
	rb_define_method(rb_cDisplayWindow, "desktop_height", _rbf rb_DisplayWindow_desktopHeight,  0);
	rb_define_method(rb_cDisplayWindow, "update",         _rbf rb_DisplayWindow_update,         0);
	rb_define_method(rb_cDisplayWindow, "present",        _rbf rb_DisplayWindow_present,        0);
	rb_define_method(rb_cDisplayWindow, "close_window",   _rbf rb_DisplayWindow_close,          0);
	rb_define_method(rb_cDisplayWindow, "should_close?",  _rbf rb_DisplayWindow_should_close,   0);
	rb_define_method(rb_cDisplayWindow, "poll_event",     _rbf rb_DisplayWindow_poll_event,     0);
}
