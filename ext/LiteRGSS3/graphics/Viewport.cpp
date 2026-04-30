// Ruby Viewport binding wrapping cgss::Viewport. The native Viewport is a
// child View of cgss::DisplayWindow — it owns its own DrawableStack and is
// drawn (with scissor + transform) by cgss::DisplayWindow::draw(). Drawables
// created with this viewport (Sprite/Text/Shape/SpriteMap) register on the
// child stack, not the window's.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "Viewport.h"
#include "Rect.h"
#include "Image.h"
#include "BlendMode.h"
#include "window/DisplayWindow.h"
#include "DrawableDisposable.h"

#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Views/DisplayWindow.h>

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

// Helper: pull (x, y, width, height) out of either an Array or a
// LiteRGSS::Rect.
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
// process-global window is used). Accept any combination.
VALUE rb_Viewport_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE a, b, c, d, e;
    rb_scan_args(argc, argv, "05", &a, &b, &c, &d, &e);
    auto *vp = get_viewport(self);
    vp->index = ++g_viewport_counter;

    if (!NIL_P(e)) {
        vp->x = NUM2INT(b);
        vp->y = NUM2INT(c);
        vp->width = NUM2INT(d);
        vp->height = NUM2INT(e);
    } else if (!NIL_P(d)) {
        vp->x = NUM2INT(a);
        vp->y = NUM2INT(b);
        vp->width = NUM2INT(c);
        vp->height = NUM2INT(d);
    } else {
        vp->x = RTEST(a) ? NUM2INT(a) : 0;
        vp->y = RTEST(b) ? NUM2INT(b) : 0;
        vp->width = RTEST(c) ? NUM2INT(c) : 640;
        vp->height = RTEST(d) ? NUM2INT(d) : 480;
    }

    auto* window = get_active_display_window();
    if (window == nullptr) {
        rb_raise(rb_eRGSSError, "Viewport.new requires an open DisplayWindow");
    }
    vp->viewport = std::make_unique<cgss::Viewport>(window->addView<cgss::Viewport>());
    vp->has_viewport = true;
    vp->viewport->move(static_cast<float>(vp->x), static_cast<float>(vp->y));
    vp->viewport->resize(vp->width, vp->height);
    return self;
}

VALUE rb_Viewport_Dispose(VALUE self)
{
    auto* vp = get_viewport(self);
    if (!vp->disposed && vp->viewport) {
        vp->viewport->detach();
        vp->disposed = true;
    }
    return self;
}
VALUE rb_Viewport_Disposed(VALUE self) { return get_viewport(self)->disposed ? Qtrue : Qfalse; }
VALUE rb_Viewport_Copy(VALUE self) { (void)self; rb_raise(rb_eRuntimeError, "Viewports cannot be cloned or duplicated."); return self; }

VALUE rb_Viewport_getOX(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->ox); }
VALUE rb_Viewport_setOX(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->ox = NUM2INT(val);
    if (vp->viewport) vp->viewport->moveOrigin(vp->ox, vp->oy);
    return val;
}
VALUE rb_Viewport_getOY(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->oy); }
VALUE rb_Viewport_setOY(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->oy = NUM2INT(val);
    if (vp->viewport) vp->viewport->moveOrigin(vp->ox, vp->oy);
    return val;
}
VALUE rb_Viewport_getVisible(VALUE self) { check_disposed(get_viewport(self)); return get_viewport(self)->visible ? Qtrue : Qfalse; }
VALUE rb_Viewport_setVisible(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->visible = RTEST(val);
    if (vp->viewport) vp->viewport->setVisible(vp->visible);
    return val;
}
VALUE rb_Viewport_getZ(VALUE self) { check_disposed(get_viewport(self)); return INT2NUM(get_viewport(self)->z); }
VALUE rb_Viewport_setZ(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->z = NUM2INT(val);
    if (vp->viewport) vp->viewport->setZ(vp->z);
    return val;
}
VALUE rb_Viewport_getZoom(VALUE self) { check_disposed(get_viewport(self)); return DBL2NUM(get_viewport(self)->zoom); }
VALUE rb_Viewport_setZoom(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->zoom = (float)NUM2DBL(val);
    if (vp->viewport) vp->viewport->setZoom(vp->zoom);
    return val;
}
VALUE rb_Viewport_getAngle(VALUE self) { check_disposed(get_viewport(self)); return DBL2NUM(get_viewport(self)->angle); }
VALUE rb_Viewport_setAngle(VALUE self, VALUE val)
{
    check_disposed(get_viewport(self));
    auto *vp = get_viewport(self);
    vp->angle = (float)(NUM2INT(val) % 360);
    if (vp->viewport) vp->viewport->setAngle(vp->angle);
    return val;
}

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
    if (vp->viewport) {
        vp->viewport->setRectangle(cgss::IntRect{ vp->x, vp->y, vp->width, vp->height });
    }
    return val;
}

VALUE rb_Viewport_index(VALUE self)
{
    return ULONG2NUM(get_viewport(self)->index);
}

// `sort_z` forwards to cgss::Viewport::sortZ — actually reorders the View's
// DrawableStack now that drawables register through Phases 2-4.
VALUE rb_Viewport_sortZ(VALUE self)
{
    auto *vp = get_viewport(self);
    if (vp->viewport) vp->viewport->sortZ();
    return self;
}

VALUE rb_Viewport_snapToBitmap(VALUE self)
{
    auto *vp = get_viewport(self);
    if (vp->viewport) {
        auto snapshot = vp->viewport->takeSnapshot();
        if (snapshot) {
            const auto sz = snapshot->getSize();
            VALUE args[2] = { UINT2NUM(sz.x), UINT2NUM(sz.y) };
            return rb_class_new_instance(2, args, rb_cImage);
        }
    }
    const long w = vp->width  > 0 ? vp->width  : 1;
    const long h = vp->height > 0 ? vp->height : 1;
    VALUE args[2] = { LONG2NUM(w), LONG2NUM(h) };
    return rb_class_new_instance(2, args, rb_cImage);
}

// Backwards-compat: pre-Phase-3 the Ruby render loop did
//   vp.begin_draw; sprite.draw; vp.end_draw
// to install scissor + transform manually. cgss::Viewport handles all of
// that during cgss::DisplayWindow::draw(); these are kept as no-ops so
// existing PSDK / test code doesn't NoMethodError.
VALUE rb_Viewport_beginDraw(VALUE self) { return self; }
VALUE rb_Viewport_endDraw(VALUE self)   { return self; }

// shader / blendmode — Ruby BlendMode (or Shader subclass) wraps a
// cgss::RenderStates. Setting one binds it on the underlying cgss::Viewport
// via bindRenderStates(); the @shader ivar pins the Ruby wrapper so its
// cgss::RenderStates outlives the binding. Both `viewport.shader=` and
// `viewport.blendmode=` route here — they share storage because cgss
// represents blend factors and shader code in the same RenderStates object.
VALUE rb_Viewport_getShader(VALUE self) { return rb_iv_get(self, "@shader"); }
VALUE rb_Viewport_setShader(VALUE self, VALUE val)
{
    auto *vp = get_viewport(self);
    check_disposed(vp);
    if (!NIL_P(val) && rb_obj_is_kind_of(val, rb_cBlendMode) != Qtrue) {
        rb_raise(rb_eRGSSError, "Viewport shader must be a BlendMode (or subclass).");
    }
    rb_iv_set(self, "@shader", val);
    if (vp->viewport) {
        if (NIL_P(val)) {
            vp->viewport->bindRenderStates(nullptr);
        } else {
            vp->viewport->bindRenderStates(rb::GetPtr<RenderStatesElement>(val));
        }
    }
    return val;
}

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
