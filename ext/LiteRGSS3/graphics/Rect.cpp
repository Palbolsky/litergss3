#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "Rect.h"

VALUE rb_cRect = Qnil;

RectData *get_rect_data(VALUE self) { return rb::GetPtr<RectData>(self); }

// Rect.new(width, height)
// Rect.new(x, width, height)        // y defaults to 0
// Rect.new(x, y, width, height)
VALUE rb_Rect_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE x, y, width, height;
    rb_scan_args(argc, argv, "22", &x, &y, &width, &height);
    auto *r = get_rect_data(self);

    if (NIL_P(width)) {
        // 2-arg form: (width, height)
        width = x;
        height = y;
        x = LONG2FIX(0);
        y = LONG2FIX(0);
    } else if (NIL_P(height)) {
        // 3-arg form: (x, width, height)
        height = width;
        width = y;
        y = LONG2FIX(0);
    }

    r->x = NUM2INT(x);
    r->y = NUM2INT(y);
    r->width = NUM2INT(width);
    r->height = NUM2INT(height);
    return self;
}

VALUE rb_Rect_InitializeCopy(VALUE self, VALUE other)
{
    *get_rect_data(self) = *get_rect_data(other);
    return self;
}

// rect.set(x[, y[, width[, height]]])  — nil keeps the existing value
VALUE rb_Rect_set(int argc, VALUE *argv, VALUE self)
{
    VALUE x, y, width, height;
    rb_scan_args(argc, argv, "13", &x, &y, &width, &height);
    auto *r = get_rect_data(self);
    if (!NIL_P(x))      r->x = NUM2INT(x);
    if (!NIL_P(y))      r->y = NUM2INT(y);
    if (!NIL_P(width))  r->width = NUM2INT(width);
    if (!NIL_P(height)) r->height = NUM2INT(height);
    return self;
}

VALUE rb_Rect_getX(VALUE self) { return INT2NUM(get_rect_data(self)->x); }
VALUE rb_Rect_getY(VALUE self) { return INT2NUM(get_rect_data(self)->y); }
VALUE rb_Rect_getWidth(VALUE self) { return INT2NUM(get_rect_data(self)->width); }
VALUE rb_Rect_getHeight(VALUE self) { return INT2NUM(get_rect_data(self)->height); }

VALUE rb_Rect_setX(VALUE self, VALUE v) { get_rect_data(self)->x = NUM2INT(v); return v; }
VALUE rb_Rect_setY(VALUE self, VALUE v) { get_rect_data(self)->y = NUM2INT(v); return v; }
VALUE rb_Rect_setWidth(VALUE self, VALUE v) { get_rect_data(self)->width = NUM2INT(v); return v; }
VALUE rb_Rect_setHeight(VALUE self, VALUE v) { get_rect_data(self)->height = NUM2INT(v); return v; }

static VALUE rb_Rect_eql_array(RectData *r, VALUE oth)
{
    if (RARRAY_LEN(oth) != 4) return Qfalse;
    if (NUM2INT(rb_ary_entry(oth, 0)) != r->x) return Qfalse;
    if (NUM2INT(rb_ary_entry(oth, 1)) != r->y) return Qfalse;
    if (NUM2INT(rb_ary_entry(oth, 2)) != r->width) return Qfalse;
    if (NUM2INT(rb_ary_entry(oth, 3)) != r->height) return Qfalse;
    return Qtrue;
}

VALUE rb_Rect_eql(VALUE self, VALUE other)
{
    auto *r = get_rect_data(self);
    if (rb_obj_is_kind_of(other, rb_cRect) == Qtrue) {
        auto *r2 = get_rect_data(other);
        return (r->x == r2->x && r->y == r2->y &&
                r->width == r2->width && r->height == r2->height) ? Qtrue : Qfalse;
    }
    if (rb_obj_is_kind_of(other, rb_cArray) == Qtrue) return rb_Rect_eql_array(r, other);
    return Qfalse;
}

VALUE rb_Rect_empty(VALUE self)
{
    auto *r = get_rect_data(self);
    r->x = r->y = r->width = r->height = 0;
    return self;
}

VALUE rb_Rect_to_s(VALUE self)
{
    auto *r = get_rect_data(self);
    return rb_sprintf("(%d, %d, %d, %d)", r->x, r->y, r->width, r->height);
}

// Marshal format mirrors litergss2: 4 packed `int`s. PSDK serializes Rect
// in save data, so this stays binary-compatible with that on-disk layout.
VALUE rb_Rect_save(VALUE self, VALUE limit)
{
    auto *r = get_rect_data(self);
    int rc[4] = { r->x, r->y, r->width, r->height };
    return rb_str_new(reinterpret_cast<const char *>(rc), sizeof(int) * 4);
}

VALUE rb_Rect_load(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    VALUE arr[4];
    if (RSTRING_LEN(str) < static_cast<long>(sizeof(int) * 4)) {
        arr[0] = arr[1] = LONG2FIX(1);
        return rb_class_new_instance(2, arr, self);
    }
    int *rc = reinterpret_cast<int *>(RSTRING_PTR(str));
    arr[0] = INT2NUM(rc[0]);
    arr[1] = INT2NUM(rc[1]);
    arr[2] = INT2NUM(rc[2]);
    arr[3] = INT2NUM(rc[3]);
    return rb_class_new_instance(4, arr, self);
}

void Init_Rect()
{
    rb_cRect = rb_define_class_under(rb_mLiteRGSS, "Rect", rb_cObject);
    rb_define_alloc_func(rb_cRect, rb::Alloc<RectData>);

    rb_define_method(rb_cRect, "initialize", _rbf rb_Rect_Initialize, -1);
    rb_define_method(rb_cRect, "initialize_copy", _rbf rb_Rect_InitializeCopy, 1);
    rb_define_method(rb_cRect, "set", _rbf rb_Rect_set, -1);
    rb_define_method(rb_cRect, "x", _rbf rb_Rect_getX, 0);
    rb_define_method(rb_cRect, "x=", _rbf rb_Rect_setX, 1);
    rb_define_method(rb_cRect, "y", _rbf rb_Rect_getY, 0);
    rb_define_method(rb_cRect, "y=", _rbf rb_Rect_setY, 1);
    rb_define_method(rb_cRect, "width", _rbf rb_Rect_getWidth, 0);
    rb_define_method(rb_cRect, "width=", _rbf rb_Rect_setWidth, 1);
    rb_define_method(rb_cRect, "height", _rbf rb_Rect_getHeight, 0);
    rb_define_method(rb_cRect, "height=", _rbf rb_Rect_setHeight, 1);
    rb_define_method(rb_cRect, "==", _rbf rb_Rect_eql, 1);
    rb_define_method(rb_cRect, "===", _rbf rb_Rect_eql, 1);
    rb_define_method(rb_cRect, "eql?", _rbf rb_Rect_eql, 1);
    rb_define_method(rb_cRect, "to_s", _rbf rb_Rect_to_s, 0);
    rb_define_method(rb_cRect, "inspect", _rbf rb_Rect_to_s, 0);
    rb_define_method(rb_cRect, "empty", _rbf rb_Rect_empty, 0);
    rb_define_method(rb_cRect, "_dump", _rbf rb_Rect_save, 1);
    rb_define_singleton_method(rb_cRect, "_load", _rbf rb_Rect_load, 1);
}
