#include "RaylibWrapper.h"
#include "LiteRGSS.h"
#include "RubyValue.h"

// Returns the number of touch points currently active
VALUE rb_Touch_count(VALUE self)
{
    return INT2NUM(raylib::GetTouchPointCount());
}

// Returns [x, y] position of a touch point by index
VALUE rb_Touch_getPosition(VALUE self, VALUE index)
{
    auto position = raylib::GetTouchPosition(NUM2INT(index));
    auto ary = rb_ary_new_capa(2);
    rb_ary_push(ary, INT2NUM((int)position.x));
    rb_ary_push(ary, INT2NUM((int)position.y));
    return ary;
}

// Returns the unique identifier of a touch point by index
VALUE rb_Touch_getId(VALUE self, VALUE index)
{
    return INT2NUM(raylib::GetTouchPointId(NUM2INT(index)));
}

void Init_Touch()
{
    VALUE rb_mTouch = rb_define_module_under(rb_mLiteRGSS, "Touch");

    rb_define_module_function(rb_mTouch, "count",        _rbf rb_Touch_count,       0);
    rb_define_module_function(rb_mTouch, "position",     _rbf rb_Touch_getPosition, 1);
    rb_define_module_function(rb_mTouch, "id",           _rbf rb_Touch_getId,       1);
}
