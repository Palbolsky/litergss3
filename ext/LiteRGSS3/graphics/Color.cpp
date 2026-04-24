// Color.cpp does NOT use any raylib symbol; the legacy RaylibWrapper
// include was transitive noise — dropped. Normalize helper lives at the
// new path under LiteCGSS/ (no legacy "2" suffix).
#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include "Color.h"

VALUE rb_cColor = Qnil;

static ColorData *get_color(VALUE self) { return rb::GetPtr<ColorData>(self); }

VALUE rb_Color_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE red, green, blue, alpha;
    rb_scan_args(argc, argv, "04", &red, &green, &blue, &alpha);
    auto *color = get_color(self);
    if (RTEST(red))
        color->r = (uint8_t)cgss::normalize_long(RB_NUM2LONG(red), 0, 255);
    if (RTEST(green))
        color->g = (uint8_t)cgss::normalize_long(RB_NUM2LONG(green), 0, 255);
    if (RTEST(blue))
        color->b = (uint8_t)cgss::normalize_long(RB_NUM2LONG(blue), 0, 255);
    if (RTEST(alpha))
        color->a = (uint8_t)cgss::normalize_long(RB_NUM2LONG(alpha), 0, 255);
    return self;
}

VALUE rb_Color_InitializeCopy(VALUE self, VALUE original)
{
    auto *dst = get_color(self);
    auto *src = get_color(original);
    *dst = *src;
    return self;
}

VALUE rb_Color_getRed(VALUE self) { return rb_int2inum(get_color(self)->r); }
VALUE rb_Color_getGreen(VALUE self) { return rb_int2inum(get_color(self)->g); }
VALUE rb_Color_getBlue(VALUE self) { return rb_int2inum(get_color(self)->b); }
VALUE rb_Color_getAlpha(VALUE self) { return rb_int2inum(get_color(self)->a); }

VALUE rb_Color_setRed(VALUE self, VALUE v)
{
    get_color(self)->r = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    return self;
}

VALUE rb_Color_setGreen(VALUE self, VALUE v)
{
    get_color(self)->g = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    return self;
}

VALUE rb_Color_setBlue(VALUE self, VALUE v)
{
    get_color(self)->b = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    return self;
}

VALUE rb_Color_setAlpha(VALUE self, VALUE v)
{
    get_color(self)->a = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    return self;
}

VALUE rb_Color_eql(VALUE self, VALUE other)
{
    if (!rb_obj_is_kind_of(other, rb_cColor))
        return Qfalse;
    return *get_color(self) == *get_color(other) ? Qtrue : Qfalse;
}

VALUE rb_Color_Save(VALUE self, VALUE limit)
{
    auto *color = get_color(self);
    double data[4] = {
        (double)color->r,
        (double)color->g,
        (double)color->b,
        (double)color->a};
    return rb_str_new(reinterpret_cast<const char *>(data), sizeof(double) * 4);
}

VALUE rb_Color_Load(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    VALUE args[4];
    if (RSTRING_LEN(str) < (long)(sizeof(double) * 4))
    {
        args[0] = args[1] = args[2] = LONG2FIX(255);
        return rb_class_new_instance(3, args, self);
    }
    double *data = reinterpret_cast<double *>(RSTRING_PTR(str));
    args[0] = rb_int2inum((long)data[0]);
    args[1] = rb_int2inum((long)data[1]);
    args[2] = rb_int2inum((long)data[2]);
    args[3] = rb_int2inum((long)data[3]);
    return rb_class_new_instance(4, args, self);
}

VALUE rb_Color_to_s(VALUE self)
{
    auto *color = get_color(self);
    return rb_sprintf("(%d, %d, %d, %d)", color->r, color->g, color->b, color->a);
}

void Init_Color()
{
    rb_cColor = rb_define_class_under(rb_mLiteRGSS, "Color", rb_cObject);
    rb_define_alloc_func(rb_cColor, rb::Alloc<ColorData>);

    rb_define_method(rb_cColor, "initialize", _rbf rb_Color_Initialize, -1);
    rb_define_method(rb_cColor, "set", _rbf rb_Color_Initialize, -1);
    rb_define_method(rb_cColor, "initialize_copy", _rbf rb_Color_InitializeCopy, 1);
    rb_define_method(rb_cColor, "red", _rbf rb_Color_getRed, 0);
    rb_define_method(rb_cColor, "red=", _rbf rb_Color_setRed, 1);
    rb_define_method(rb_cColor, "green", _rbf rb_Color_getGreen, 0);
    rb_define_method(rb_cColor, "green=", _rbf rb_Color_setGreen, 1);
    rb_define_method(rb_cColor, "blue", _rbf rb_Color_getBlue, 0);
    rb_define_method(rb_cColor, "blue=", _rbf rb_Color_setBlue, 1);
    rb_define_method(rb_cColor, "alpha", _rbf rb_Color_getAlpha, 0);
    rb_define_method(rb_cColor, "alpha=", _rbf rb_Color_setAlpha, 1);
    rb_define_method(rb_cColor, "==", _rbf rb_Color_eql, 1);
    rb_define_method(rb_cColor, "===", _rbf rb_Color_eql, 1);
    rb_define_method(rb_cColor, "eql?", _rbf rb_Color_eql, 1);
    rb_define_method(rb_cColor, "to_s", _rbf rb_Color_to_s, 0);
    rb_define_method(rb_cColor, "inspect", _rbf rb_Color_to_s, 0);
    rb_define_method(rb_cColor, "_dump", _rbf rb_Color_Save, 1);
    rb_define_singleton_method(rb_cColor, "_load", _rbf rb_Color_Load, 1);
}
