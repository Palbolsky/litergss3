// Backend-agnostic Ruby Viewport binding. begin_draw / end_draw route
// through Ops::viewport_begin / viewport_end — raylib: BeginScissorMode +
// rlPushMatrix+translate+scale; SFML: sf::View with viewport+size+center
// emulation (see SFMLBackend.h).

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "Viewport.h"
#include "window/Window.h"

#include <LiteCGSS/Backend/ActiveBackend.h>

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

VALUE rb_cViewport = Qnil;

ViewportData *get_viewport(VALUE self) { return rb::GetPtr<ViewportData>(self); }

static void check_disposed(ViewportData *vp)
{
    if (vp->disposed)
        rb_raise(rb_eRuntimeError, "Viewport is disposed");
}

static float get_window_scale()
{
    return (float)window_scale;
}

// --- Methods ---

VALUE rb_Viewport_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE x, y, w, h;
    rb_scan_args(argc, argv, "04", &x, &y, &w, &h);
    auto *vp = get_viewport(self);
    vp->x = RTEST(x) ? NUM2INT(x) : 0;
    vp->y = RTEST(y) ? NUM2INT(y) : 0;
    vp->width = RTEST(w) ? NUM2INT(w) : 640;
    vp->height = RTEST(h) ? NUM2INT(h) : 480;
    return self;
}

VALUE rb_Viewport_Dispose(VALUE self) { get_viewport(self)->disposed = true; return self; }
VALUE rb_Viewport_Disposed(VALUE self) { return get_viewport(self)->disposed ? Qtrue : Qfalse; }
VALUE rb_Viewport_Copy(VALUE self) { (void)self; rb_raise(rb_eRuntimeError, "Viewports cannot be cloned or duplicated."); return self; }

VALUE rb_Viewport_getOX(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->ox); }
VALUE rb_Viewport_setOX(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->ox = NUM2INT(val); return val; }
VALUE rb_Viewport_getOY(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->oy); }
VALUE rb_Viewport_setOY(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->oy = NUM2INT(val); return val; }
VALUE rb_Viewport_getVisible(VALUE self) { check_disposed(get_viewport(self)); return get_viewport(self)->visible ? Qtrue : Qfalse; }
VALUE rb_Viewport_setVisible(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->visible = RTEST(val); return val; }
VALUE rb_Viewport_getZ(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->z); }
VALUE rb_Viewport_setZ(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->z = NUM2INT(val); return val; }
VALUE rb_Viewport_getZoom(VALUE self) { check_disposed(get_viewport(self)); return DBL2NUM(get_viewport(self)->zoom); }
VALUE rb_Viewport_setZoom(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->zoom = (float)NUM2DBL(val); return val; }
VALUE rb_Viewport_getAngle(VALUE self) { check_disposed(get_viewport(self)); return DBL2NUM(get_viewport(self)->angle); }
VALUE rb_Viewport_setAngle(VALUE self, VALUE val) { check_disposed(get_viewport(self)); get_viewport(self)->angle = (float)(NUM2INT(val) % 360); return val; }

VALUE rb_Viewport_getRect(VALUE self)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    VALUE ary = rb_ary_new_capa(4);
    rb_ary_push(ary, INT2NUM(vp->x));
    rb_ary_push(ary, INT2NUM(vp->y));
    rb_ary_push(ary, INT2NUM(vp->width));
    rb_ary_push(ary, INT2NUM(vp->height));
    return ary;
}

VALUE rb_Viewport_setRect(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    Check_Type(val, T_ARRAY);
    auto *vp = get_viewport(self);
    vp->x = NUM2INT(rb_ary_entry(val, 0));
    vp->y = NUM2INT(rb_ary_entry(val, 1));
    vp->width = NUM2INT(rb_ary_entry(val, 2));
    vp->height = NUM2INT(rb_ary_entry(val, 3));
    return val;
}

VALUE rb_Viewport_beginDraw(VALUE self)
{
    auto *vp = get_viewport(self);
    if (vp->disposed || !vp->visible) return self;

    auto* window = get_active_native_window();
    if (window == nullptr) return self;
    auto& target = Ops::target_from_window(*window);

    const float scale = get_window_scale();

    const int scaled_x = (int)(vp->x * scale);
    const int scaled_y = (int)(vp->y * scale);
    const int scaled_width  = (int)(vp->width  * scale);
    const int scaled_height = (int)(vp->height * scale);

    Ops::viewport_begin(target,
                        scaled_x, scaled_y, scaled_width, scaled_height,
                        -vp->ox * scale, -vp->oy * scale,
                        scale * vp->zoom, scale * vp->zoom);

    // Debug/background fill preserved from the legacy raylib-direct impl.
    // Sprites drawn later overlay on top.
    Ops::draw_filled_rect(target,
                          scaled_x, scaled_y, scaled_width, scaled_height,
                          cgss::Colors::White);

    return self;
}

VALUE rb_Viewport_endDraw(VALUE self)
{
    auto* window = get_active_native_window();
    if (window == nullptr) return self;
    auto& target = Ops::target_from_window(*window);
    Ops::viewport_end(target);
    return self;
}

// --- Init ---

void Init_Viewport()
{
    rb_cViewport = rb_define_class_under(rb_mLiteRGSS, "Viewport", rb_cObject);
    rb_define_alloc_func(rb_cViewport, rb::Alloc<ViewportData>);

    rb_define_method(rb_cViewport, "initialize", _rbf rb_Viewport_Initialize, -1);
    rb_define_method(rb_cViewport, "dispose", _rbf rb_Viewport_Dispose, 0);
    rb_define_method(rb_cViewport, "disposed?", _rbf rb_Viewport_Disposed, 0);
    rb_define_method(rb_cViewport, "clone", _rbf rb_Viewport_Copy, 0);
    rb_define_method(rb_cViewport, "dup", _rbf rb_Viewport_Copy, 0);
    rb_define_method(rb_cViewport, "ox", _rbf rb_Viewport_getOX, 0);
    rb_define_method(rb_cViewport, "ox=", _rbf rb_Viewport_setOX, 1);
    rb_define_method(rb_cViewport, "oy", _rbf rb_Viewport_getOY, 0);
    rb_define_method(rb_cViewport, "oy=", _rbf rb_Viewport_setOY, 1);
    rb_define_method(rb_cViewport, "visible", _rbf rb_Viewport_getVisible, 0);
    rb_define_method(rb_cViewport, "visible=", _rbf rb_Viewport_setVisible, 1);
    rb_define_method(rb_cViewport, "z", _rbf rb_Viewport_getZ, 0);
    rb_define_method(rb_cViewport, "z=", _rbf rb_Viewport_setZ, 1);
    rb_define_method(rb_cViewport, "zoom", _rbf rb_Viewport_getZoom, 0);
    rb_define_method(rb_cViewport, "zoom=", _rbf rb_Viewport_setZoom, 1);
    rb_define_method(rb_cViewport, "angle", _rbf rb_Viewport_getAngle, 0);
    rb_define_method(rb_cViewport, "angle=", _rbf rb_Viewport_setAngle, 1);
    rb_define_method(rb_cViewport, "rect", _rbf rb_Viewport_getRect, 0);
    rb_define_method(rb_cViewport, "rect=", _rbf rb_Viewport_setRect, 1);
    rb_define_method(rb_cViewport, "begin_draw", _rbf rb_Viewport_beginDraw, 0);
    rb_define_method(rb_cViewport, "end_draw", _rbf rb_Viewport_endDraw, 0);
}
