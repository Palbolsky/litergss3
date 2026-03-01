#include "Window.h"
#include "LiteRGSS.h"

VALUE rb_cWindow = Qnil;

// open_window(width=640, height=480, title="LiteRGSS")
VALUE rb_Window_open(int argc, VALUE* argv, VALUE self)
{
    VALUE width, height, title;
    rb_scan_args(argc, argv, "03", &width, &height, &title);

    int   w = RTEST(width)  ? NUM2INT(width)            : 640;
    int   h = RTEST(height) ? NUM2INT(height)           : 480;
    const char* t = RTEST(title) ? StringValueCStr(title) : "LiteRGSS";

    raylib::InitWindow(w, h, t);
    return self;
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

    rb_define_method(rb_cWindow, "open_window",   _rbf rb_Window_open,         -1);
    rb_define_method(rb_cWindow, "update",        _rbf rb_Window_update,        0);
    rb_define_method(rb_cWindow, "close_window",  _rbf rb_Window_close,         0);
    rb_define_method(rb_cWindow, "should_close?", _rbf rb_Window_should_close,  0);
}
