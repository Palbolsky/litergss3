// Backend-agnostic Ruby Viewport binding. begin_draw / end_draw route
// through Ops::viewport_begin / viewport_end — raylib: BeginScissorMode +
// rlPushMatrix+translate+scale; SFML: sf::View with viewport+size+center
// emulation (see SFMLBackend.h).

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "Viewport.h"
#include "Rect.h"
#include "Image.h"
#include "window/DisplayWindow.h"
#include "DrawableDisposable.h"

#include <LiteCGSS/Backend/ActiveBackend.h>

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

VALUE rb_cViewport = Qnil;

// Monotonic counter used to generate __index__ values. Incremented in
// rb_Viewport_Initialize regardless of Ruby args, so two viewports with
// identical z still sort deterministically by creation order.
static unsigned long g_viewport_counter = 0;

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

// Helper: pull (x, y, width, height) out of either an Array or a
// LiteRGSS::Rect. Used by `rect=` and the multi-arg forms below.
static void rect_arg_to_xywh(VALUE val, int& x, int& y, int& w, int& h)
{
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *r = get_rect_data(val);
        x = r->x; y = r->y; w = r->width; h = r->height;
        return;
    }
    Check_Type(val, T_ARRAY);
    if (RARRAY_LEN(val) < 4) {
        rb_raise(rb_eArgError, "Rect array must have 4 elements");
    }
    x = NUM2INT(rb_ary_entry(val, 0));
    y = NUM2INT(rb_ary_entry(val, 1));
    w = NUM2INT(rb_ary_entry(val, 2));
    h = NUM2INT(rb_ary_entry(val, 3));
}

// --- Methods ---

// LiteRGSS2 viewport ctor was `Viewport.new(window, x, y, w, h)`. PSDK
// still passes the leading `window` arg; LiteRGSS3 ignores it (the
// process-global window is used by begin_draw). Accept any combination so
// both calling conventions work.
VALUE rb_Viewport_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE a, b, c, d, e;
    rb_scan_args(argc, argv, "05", &a, &b, &c, &d, &e);
    auto *vp = get_viewport(self);
    vp->index = ++g_viewport_counter;

    // 5-arg form: LiteRGSS2 `(window, x, y, w, h)` — drop `window`.
    if (!NIL_P(e)) {
        vp->x = NUM2INT(b);
        vp->y = NUM2INT(c);
        vp->width = NUM2INT(d);
        vp->height = NUM2INT(e);
        return self;
    }
    // 4-arg form: native `(x, y, w, h)`.
    if (!NIL_P(d)) {
        vp->x = NUM2INT(a);
        vp->y = NUM2INT(b);
        vp->width = NUM2INT(c);
        vp->height = NUM2INT(d);
        return self;
    }
    // No-arg / partial — defaults from the legacy LiteRGSS3 behaviour.
    vp->x = RTEST(a) ? NUM2INT(a) : 0;
    vp->y = RTEST(b) ? NUM2INT(b) : 0;
    vp->width = RTEST(c) ? NUM2INT(c) : 640;
    vp->height = RTEST(d) ? NUM2INT(d) : 480;
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

// LiteRGSS2 returned a Rect instance, not an Array. Match that.
VALUE rb_Viewport_getRect(VALUE self)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    VALUE args[4] = { INT2NUM(vp->x), INT2NUM(vp->y), INT2NUM(vp->width), INT2NUM(vp->height) };
    return rb_class_new_instance(4, args, rb_cRect);
}

VALUE rb_Viewport_setRect(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    rect_arg_to_xywh(val, vp->x, vp->y, vp->width, vp->height);
    return val;
}

VALUE rb_Viewport_index(VALUE self)
{
    return ULONG2NUM(get_viewport(self)->index);
}

// LiteRGSS2 sorted the internal child-draw list here. LiteRGSS3 has no
// such list — drawing is explicit per-frame. Keep the method as a no-op
// so PSDK's `viewport.sort_z` calls don't blow up.
VALUE rb_Viewport_sortZ(VALUE self) { return self; }

// LiteRGSS2 returned a GPU texture snapshot of the viewport. Until that
// path is plumbed in LiteRGSS3, hand back an empty Image of the right
// dimensions — PSDK's screenshot/transition paths just need something to
// dispose later.
VALUE rb_Viewport_snapToBitmap(VALUE self)
{
    auto *vp = get_viewport(self);
    const long w = vp->width  > 0 ? vp->width  : 1;
    const long h = vp->height > 0 ? vp->height : 1;
    VALUE args[2] = { LONG2NUM(w), LONG2NUM(h) };
    return rb_class_new_instance(2, args, rb_cImage);
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

// shader / blendmode — Ruby-level instance variable storage. Match the
// litergss2 surface (PSDK Graphics.rb#snap_to_bitmap reads viewport.shader
// to detect when transition effects are active). Actual binding to the
// render pipeline lands once Viewport hosts a cgss DrawableStack.
VALUE rb_Viewport_getShader(VALUE self) { return rb_iv_get(self, "@shader"); }
VALUE rb_Viewport_setShader(VALUE self, VALUE val) { rb_iv_set(self, "@shader", val); return val; }

// --- Init ---

void Init_Viewport()
{
    rb_cViewport = rb_define_class_under(rb_mLiteRGSS, "Viewport", rb_cDrawable);
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
    rb_define_method(rb_cViewport, "shader", _rbf rb_Viewport_getShader, 0);
    rb_define_method(rb_cViewport, "shader=", _rbf rb_Viewport_setShader, 1);
    rb_define_method(rb_cViewport, "blendmode", _rbf rb_Viewport_getShader, 0);
    rb_define_method(rb_cViewport, "blendmode=", _rbf rb_Viewport_setShader, 1);
    rb_define_method(rb_cViewport, "__index__", _rbf rb_Viewport_index, 0);
    rb_define_method(rb_cViewport, "sort_z", _rbf rb_Viewport_sortZ, 0);
    rb_define_method(rb_cViewport, "snap_to_bitmap", _rbf rb_Viewport_snapToBitmap, 0);
}
