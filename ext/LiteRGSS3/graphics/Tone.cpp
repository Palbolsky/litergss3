#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include "Tone.h"

VALUE rb_cTone = Qnil;

ToneData *get_tone_data(VALUE self) { return rb::GetPtr<ToneData>(self); }

VALUE rb_Tone_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE red, green, blue, gray;
    rb_scan_args(argc, argv, "13", &red, &green, &blue, &gray);
    auto *t = get_tone_data(self);
    if (RTEST(red))   t->r    = cgss::normalize_long(RB_NUM2LONG(red), -255, 255) / 255.0f;
    if (RTEST(green)) t->g    = cgss::normalize_long(RB_NUM2LONG(green), -255, 255) / 255.0f;
    if (RTEST(blue))  t->b    = cgss::normalize_long(RB_NUM2LONG(blue), -255, 255) / 255.0f;
    if (RTEST(gray))  t->gray = cgss::normalize_long(RB_NUM2LONG(gray), 0, 255) / 255.0f;
    return self;
}

VALUE rb_Tone_InitializeCopy(VALUE self, VALUE other)
{
    *get_tone_data(self) = *get_tone_data(other);
    return self;
}

VALUE rb_Tone_getRed(VALUE self) { return INT2NUM(static_cast<long>(get_tone_data(self)->r * 255.0f)); }
VALUE rb_Tone_getGreen(VALUE self) { return INT2NUM(static_cast<long>(get_tone_data(self)->g * 255.0f)); }
VALUE rb_Tone_getBlue(VALUE self) { return INT2NUM(static_cast<long>(get_tone_data(self)->b * 255.0f)); }
VALUE rb_Tone_getGray(VALUE self) { return INT2NUM(static_cast<long>(get_tone_data(self)->gray * 255.0f)); }

VALUE rb_Tone_setRed(VALUE self, VALUE v)
{
    get_tone_data(self)->r = cgss::normalize_long(RB_NUM2LONG(v), -255, 255) / 255.0f;
    return v;
}
VALUE rb_Tone_setGreen(VALUE self, VALUE v)
{
    get_tone_data(self)->g = cgss::normalize_long(RB_NUM2LONG(v), -255, 255) / 255.0f;
    return v;
}
VALUE rb_Tone_setBlue(VALUE self, VALUE v)
{
    get_tone_data(self)->b = cgss::normalize_long(RB_NUM2LONG(v), -255, 255) / 255.0f;
    return v;
}
VALUE rb_Tone_setGray(VALUE self, VALUE v)
{
    get_tone_data(self)->gray = cgss::normalize_long(RB_NUM2LONG(v), 0, 255) / 255.0f;
    return v;
}

VALUE rb_Tone_eql(VALUE self, VALUE other)
{
    if (rb_obj_is_kind_of(other, rb_cTone) != Qtrue) return Qfalse;
    auto *a = get_tone_data(self);
    auto *b = get_tone_data(other);
    return (a->r == b->r && a->g == b->g && a->b == b->b && a->gray == b->gray) ? Qtrue : Qfalse;
}

VALUE rb_Tone_to_s(VALUE self)
{
    auto *t = get_tone_data(self);
    return rb_sprintf("(%d, %d, %d, %d)",
                      static_cast<int>(t->r * 255),
                      static_cast<int>(t->g * 255),
                      static_cast<int>(t->b * 255),
                      static_cast<int>(t->gray * 255));
}

// On-disk format kept identical to litergss2: 4 doubles in 0..255 range.
VALUE rb_Tone_save(VALUE self, VALUE limit)
{
    auto *t = get_tone_data(self);
    double data[4] = {
        static_cast<double>(t->r * 255),
        static_cast<double>(t->g * 255),
        static_cast<double>(t->b * 255),
        static_cast<double>(t->gray * 255) };
    return rb_str_new(reinterpret_cast<const char *>(data), sizeof(double) * 4);
}

VALUE rb_Tone_load(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    VALUE arr[4];
    if (RSTRING_LEN(str) < static_cast<long>(sizeof(double) * 4)) {
        arr[0] = arr[1] = arr[2] = arr[3] = LONG2FIX(0);
        return rb_class_new_instance(4, arr, self);
    }
    double *data = reinterpret_cast<double *>(RSTRING_PTR(str));
    arr[0] = INT2NUM(static_cast<long>(data[0]));
    arr[1] = INT2NUM(static_cast<long>(data[1]));
    arr[2] = INT2NUM(static_cast<long>(data[2]));
    arr[3] = INT2NUM(static_cast<long>(data[3]));
    return rb_class_new_instance(4, arr, self);
}

void Init_Tone()
{
    rb_cTone = rb_define_class_under(rb_mLiteRGSS, "Tone", rb_cObject);
    rb_define_alloc_func(rb_cTone, rb::Alloc<ToneData>);

    rb_define_method(rb_cTone, "initialize", _rbf rb_Tone_Initialize, -1);
    rb_define_method(rb_cTone, "set", _rbf rb_Tone_Initialize, -1);
    rb_define_method(rb_cTone, "initialize_copy", _rbf rb_Tone_InitializeCopy, 1);
    rb_define_method(rb_cTone, "red", _rbf rb_Tone_getRed, 0);
    rb_define_method(rb_cTone, "red=", _rbf rb_Tone_setRed, 1);
    rb_define_method(rb_cTone, "green", _rbf rb_Tone_getGreen, 0);
    rb_define_method(rb_cTone, "green=", _rbf rb_Tone_setGreen, 1);
    rb_define_method(rb_cTone, "blue", _rbf rb_Tone_getBlue, 0);
    rb_define_method(rb_cTone, "blue=", _rbf rb_Tone_setBlue, 1);
    rb_define_method(rb_cTone, "gray", _rbf rb_Tone_getGray, 0);
    rb_define_method(rb_cTone, "gray=", _rbf rb_Tone_setGray, 1);
    rb_define_method(rb_cTone, "==", _rbf rb_Tone_eql, 1);
    rb_define_method(rb_cTone, "===", _rbf rb_Tone_eql, 1);
    rb_define_method(rb_cTone, "eql?", _rbf rb_Tone_eql, 1);
    rb_define_method(rb_cTone, "to_s", _rbf rb_Tone_to_s, 0);
    rb_define_method(rb_cTone, "inspect", _rbf rb_Tone_to_s, 0);
    rb_define_method(rb_cTone, "_dump", _rbf rb_Tone_save, 1);
    rb_define_singleton_method(rb_cTone, "_load", _rbf rb_Tone_load, 1);
}
