#include "Window.h"
#include "LiteRGSS.h"

VALUE rb_cWindow = Qnil;

void rb_Open_Window()
{
    raylib::InitWindow(640, 480, "raylib");

    while (!raylib::WindowShouldClose())
    {
        raylib::BeginDrawing();
        raylib::EndDrawing();
    }

    raylib::CloseWindow();
}

void Init_Window()
{
    rb_cWindow = rb_define_class_under(rb_mLiteRGSS, "Window", rb_cObject);

    rb_define_method(rb_cWindow, "open_window", _rbf rb_Open_Window, 0);
}