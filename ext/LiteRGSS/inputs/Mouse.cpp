#include <LiteCGSS2/Common/RaylibWrapper.h>
#include "Inputs.h"
#include "RubyValue.h"

VALUE rb_Mouse_press(VALUE self, VALUE button)
{
    auto vbutton = NUM2INT(button);
    return raylib::IsMouseButtonDown(vbutton) ? Qtrue : Qfalse;
}

VALUE rb_Mouse_getPosition(VALUE self)
{
    auto position = raylib::GetMousePosition();
    auto ary = rb_ary_new_capa(2);
    rb_ary_push(ary, INT2NUM((int)position.x));
    rb_ary_push(ary, INT2NUM((int)position.y));
    return ary;
}

VALUE rb_Mouse_setPosition(VALUE self, VALUE x, VALUE y)
{
    raylib::SetMousePosition(NUM2INT(x), NUM2INT(y));
    return self;
}

VALUE rb_Mouse_wheelMove(VALUE self)
{
    return DBL2NUM(raylib::GetMouseWheelMove());
}

void Init_Mouse()
{
    VALUE rb_mMouse = rb_define_module_under(rb_mInputs, "Mouse");

    rb_define_const(rb_mMouse, "Left", INT2NUM(raylib::MOUSE_BUTTON_LEFT));
    rb_define_const(rb_mMouse, "Right", INT2NUM(raylib::MOUSE_BUTTON_RIGHT));
    rb_define_const(rb_mMouse, "Middle", INT2NUM(raylib::MOUSE_BUTTON_MIDDLE));
    rb_define_const(rb_mMouse, "XButton1", INT2NUM(raylib::MOUSE_BUTTON_EXTRA));
    rb_define_const(rb_mMouse, "XButton2", INT2NUM(raylib::MOUSE_BUTTON_SIDE));
    rb_define_const(rb_mMouse, "XButton3", INT2NUM(raylib::MOUSE_BUTTON_FORWARD));
    rb_define_const(rb_mMouse, "VerticalWheel", INT2NUM(10));
    rb_define_const(rb_mMouse, "HorizontalWheel", INT2NUM(11));

    rb_define_module_function(rb_mMouse, "press?", _rbf rb_Mouse_press, 1);
    rb_define_module_function(rb_mMouse, "position", _rbf rb_Mouse_getPosition, 0);
    rb_define_module_function(rb_mMouse, "set_position", _rbf rb_Mouse_setPosition, 2);
    rb_define_module_function(rb_mMouse, "wheel_move", _rbf rb_Mouse_wheelMove, 0);
}
