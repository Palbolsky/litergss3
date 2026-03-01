#include "Window.h"
#include "LiteRGSS.h"

VALUE rb_cWindow = Qnil;

static int base_width  = 640;
static int base_height = 480;

// open_window(width=640, height=480, title="LiteRGSS", scale=1)
VALUE rb_Window_open(int argc, VALUE* argv, VALUE self)
{
    VALUE width, height, title, scale;
    rb_scan_args(argc, argv, "04", &width, &height, &title, &scale);

    base_width  = RTEST(width)  ? NUM2INT(width)  : 640;
    base_height = RTEST(height) ? NUM2INT(height) : 480;

    const char* t = RTEST(title) ? StringValueCStr(title) : "LiteRGSS";
    double      s = RTEST(scale) ? NUM2DBL(scale)         : 1.0;

    int w = (int)(base_width  * s);
    int h = (int)(base_height * s);

    raylib::InitWindow(w, h, t);
    return self;
}

VALUE rb_Window_scale(VALUE self, VALUE scale)
{
    double s = NUM2DBL(scale);
    int w = (int)(base_width  * s);
    int h = (int)(base_height * s);
    raylib::SetWindowSize(w, h);
    return self;
}

VALUE rb_Window_resizeScreen(VALUE self, VALUE width, VALUE height)
{
    base_width  = NUM2INT(width);
    base_height = NUM2INT(height);
    raylib::SetWindowSize(base_width, base_height);
    return self;
}

VALUE rb_Window_setTitle(VALUE self, VALUE title)
{
    raylib::SetWindowTitle(StringValueCStr(title));
    return self;
}

VALUE rb_Window_setIcon(VALUE self, VALUE path)
{
    const char* filepath = StringValueCStr(path);
    raylib::Image icon = raylib_LoadImage(filepath);

    if (icon.data == nullptr)
    {
        rb_raise(rb_eRuntimeError, "Failed to load icon: %s", filepath);
        return self;
    }

    raylib::SetWindowIcon(icon);
    raylib_UnloadImage(icon);
    return self;
}

VALUE rb_Window_setVsync(VALUE self, VALUE enabled)
{
    if (RTEST(enabled))
        raylib::SetConfigFlags(raylib::FLAG_VSYNC_HINT);
    else
        raylib::SetTargetFPS(60);
    return self;
}

VALUE rb_Window_setFps(VALUE self, VALUE fps)
{
    raylib::SetTargetFPS(NUM2INT(fps));
    return self;
}

VALUE rb_Window_getX(VALUE self)
{
    return INT2NUM(raylib::GetWindowPosition().x);
}

VALUE rb_Window_getY(VALUE self)
{
    return INT2NUM(raylib::GetWindowPosition().y);
}

VALUE rb_Window_move(VALUE self, VALUE x, VALUE y)
{
    raylib::SetWindowPosition(NUM2INT(x), NUM2INT(y));
    return self;
}

VALUE rb_Window_desktopWidth(VALUE self)
{
    return INT2NUM(raylib::GetMonitorWidth(raylib::GetCurrentMonitor()));
}

VALUE rb_Window_desktopHeight(VALUE self)
{
    return INT2NUM(raylib::GetMonitorHeight(raylib::GetCurrentMonitor()));
}

VALUE rb_Window_update(VALUE self)
{
    raylib::BeginDrawing();
    raylib::EndDrawing();
    return raylib::WindowShouldClose() ? Qtrue : Qfalse;
}

VALUE rb_Window_close(VALUE self)
{
    raylib::CloseWindow();
    return self;
}

VALUE rb_Window_should_close(VALUE self)
{
    return raylib::WindowShouldClose() ? Qtrue : Qfalse;
}

void Init_Window()
{
    rb_cWindow = rb_define_class_under(rb_mLiteRGSS, "Window", rb_cObject);

    rb_define_method(rb_cWindow, "open_window",    _rbf rb_Window_open,          -1);
    rb_define_method(rb_cWindow, "scale_window",   _rbf rb_Window_scale,          1);
    rb_define_method(rb_cWindow, "resize_screen",  _rbf rb_Window_resizeScreen,   2);
    rb_define_method(rb_cWindow, "set_title",      _rbf rb_Window_setTitle,       1);
    rb_define_method(rb_cWindow, "set_icon",       _rbf rb_Window_setIcon,        1);
    rb_define_method(rb_cWindow, "set_vsync",      _rbf rb_Window_setVsync,       1);
    rb_define_method(rb_cWindow, "set_fps",        _rbf rb_Window_setFps,         1);
    rb_define_method(rb_cWindow, "x",              _rbf rb_Window_getX,           0);
    rb_define_method(rb_cWindow, "y",              _rbf rb_Window_getY,           0);
    rb_define_method(rb_cWindow, "move",           _rbf rb_Window_move,           2);
    rb_define_method(rb_cWindow, "desktop_width",  _rbf rb_Window_desktopWidth,   0);
    rb_define_method(rb_cWindow, "desktop_height", _rbf rb_Window_desktopHeight,  0);
    rb_define_method(rb_cWindow, "update",         _rbf rb_Window_update,         0);
    rb_define_method(rb_cWindow, "close_window",   _rbf rb_Window_close,          0);
    rb_define_method(rb_cWindow, "should_close?",  _rbf rb_Window_should_close,   0);
}