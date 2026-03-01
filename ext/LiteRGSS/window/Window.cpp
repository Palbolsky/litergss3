#include "Window.h"
#include "LiteRGSS.h"

VALUE rb_cWindow = Qnil;

VALUE rb_Window_open(VALUE self)
{
    raylib::InitWindow(640, 480, "LiteRGSS");
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

    rb_define_method(rb_cWindow, "open_window",    _rbf rb_Window_open,         0);
    rb_define_method(rb_cWindow, "update",         _rbf rb_Window_update,       0);
    rb_define_method(rb_cWindow, "close_window",   _rbf rb_Window_close,        0);
    rb_define_method(rb_cWindow, "should_close?",  _rbf rb_Window_should_close, 0);
}
