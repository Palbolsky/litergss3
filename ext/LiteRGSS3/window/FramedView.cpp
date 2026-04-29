// LiteRGSS::Window — FramedView UI widget. State-only implementation; see
// header for the cgss integration limitation.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include "FramedView.h"
#include "DrawableDisposable.h"

VALUE rb_cFramedView = Qnil;

namespace rb {
    template <>
    inline void Mark<FramedViewData>(void *ptr)
    {
        auto *w = static_cast<FramedViewData *>(ptr);
        if (w == nullptr) return;
        rb_gc_mark(w->rViewport);
        rb_gc_mark(w->rWindowskin);
        rb_gc_mark(w->rCursorskin);
        rb_gc_mark(w->rPauseskin);
        rb_gc_mark(w->rWindowBuilder);
        rb_gc_mark(w->rCursorRect);
    }
}

static FramedViewData *get_fv(VALUE self) { return rb::GetPtr<FramedViewData>(self); }

VALUE rb_FramedView_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE viewport;
    rb_scan_args(argc, argv, "01", &viewport);
    get_fv(self)->rViewport = NIL_P(viewport) ? Qnil : viewport;
    return self;
}

VALUE rb_FramedView_Dispose(VALUE self) { get_fv(self)->disposed = true; return self; }
VALUE rb_FramedView_Disposed(VALUE self) { return get_fv(self)->disposed ? Qtrue : Qfalse; }
VALUE rb_FramedView_Update(VALUE self) { return self; }
VALUE rb_FramedView_SortZ(VALUE self) { return self; }

#define DEFINE_INT_ACCESSOR(field) \
    VALUE rb_FramedView_get_##field(VALUE self) { return INT2NUM(get_fv(self)->field); } \
    VALUE rb_FramedView_set_##field(VALUE self, VALUE val) { get_fv(self)->field = NUM2INT(val); return val; }

DEFINE_INT_ACCESSOR(x)
DEFINE_INT_ACCESSOR(y)
DEFINE_INT_ACCESSOR(z)
DEFINE_INT_ACCESSOR(width)
DEFINE_INT_ACCESSOR(height)
DEFINE_INT_ACCESSOR(ox)
DEFINE_INT_ACCESSOR(oy)
DEFINE_INT_ACCESSOR(pause_x)
DEFINE_INT_ACCESSOR(pause_y)

#undef DEFINE_INT_ACCESSOR

#define DEFINE_OPACITY(field) \
    VALUE rb_FramedView_get_##field(VALUE self) { return INT2NUM(get_fv(self)->field); } \
    VALUE rb_FramedView_set_##field(VALUE self, VALUE val) \
    { \
        get_fv(self)->field = static_cast<uint8_t>(cgss::normalize_long(NUM2LONG(val), 0, 255)); \
        return val; \
    }
DEFINE_OPACITY(opacity)
DEFINE_OPACITY(back_opacity)
DEFINE_OPACITY(contents_opacity)
#undef DEFINE_OPACITY

#define DEFINE_BOOL_ACCESSOR(field) \
    VALUE rb_FramedView_get_##field(VALUE self) { return get_fv(self)->field ? Qtrue : Qfalse; } \
    VALUE rb_FramedView_set_##field(VALUE self, VALUE val) { get_fv(self)->field = RTEST(val); return val; }

DEFINE_BOOL_ACCESSOR(active)
DEFINE_BOOL_ACCESSOR(stretch)
DEFINE_BOOL_ACCESSOR(visible)

#undef DEFINE_BOOL_ACCESSOR

VALUE rb_FramedView_get_pause(VALUE self) { return get_fv(self)->paused ? Qtrue : Qfalse; }
VALUE rb_FramedView_set_pause(VALUE self, VALUE val) { get_fv(self)->paused = RTEST(val); return val; }

VALUE rb_FramedView_setSize(VALUE self, VALUE w, VALUE h)
{
    auto *f = get_fv(self);
    f->width = NUM2INT(w); f->height = NUM2INT(h); return self;
}
VALUE rb_FramedView_setPosition(VALUE self, VALUE x, VALUE y)
{
    auto *f = get_fv(self);
    f->x = NUM2INT(x); f->y = NUM2INT(y); return self;
}
VALUE rb_FramedView_setOrigin(VALUE self, VALUE x, VALUE y)
{
    auto *f = get_fv(self);
    f->ox = NUM2INT(x); f->oy = NUM2INT(y); return self;
}

VALUE rb_FramedView_get_skin(VALUE self) { return get_fv(self)->rWindowskin; }
VALUE rb_FramedView_set_skin(VALUE self, VALUE val) { get_fv(self)->rWindowskin = val; return val; }
VALUE rb_FramedView_get_cursorskin(VALUE self) { return get_fv(self)->rCursorskin; }
VALUE rb_FramedView_set_cursorskin(VALUE self, VALUE val) { get_fv(self)->rCursorskin = val; return val; }
VALUE rb_FramedView_get_pauseskin(VALUE self) { return get_fv(self)->rPauseskin; }
VALUE rb_FramedView_set_pauseskin(VALUE self, VALUE val) { get_fv(self)->rPauseskin = val; return val; }
VALUE rb_FramedView_get_window_builder(VALUE self) { return get_fv(self)->rWindowBuilder; }
VALUE rb_FramedView_set_window_builder(VALUE self, VALUE val) { get_fv(self)->rWindowBuilder = val; return val; }
VALUE rb_FramedView_get_cursor_rect(VALUE self) { return get_fv(self)->rCursorRect; }
VALUE rb_FramedView_set_cursor_rect(VALUE self, VALUE val) { get_fv(self)->rCursorRect = val; return val; }

VALUE rb_FramedView_rect(VALUE self)
{
    auto *f = get_fv(self);
    VALUE arr = rb_ary_new_capa(4);
    rb_ary_push(arr, INT2NUM(f->x));
    rb_ary_push(arr, INT2NUM(f->y));
    rb_ary_push(arr, INT2NUM(f->width));
    rb_ary_push(arr, INT2NUM(f->height));
    return arr;
}

VALUE rb_FramedView_viewport(VALUE self) { return get_fv(self)->rViewport; }

VALUE rb_FramedView_lock(VALUE self) { get_fv(self)->locked = true; return self; }
VALUE rb_FramedView_unlock(VALUE self) { get_fv(self)->locked = false; return self; }
VALUE rb_FramedView_locked(VALUE self) { return get_fv(self)->locked ? Qtrue : Qfalse; }
VALUE rb_FramedView_index(VALUE self) { (void)self; return INT2NUM(0); }

void Init_FramedView()
{
    // Registers as `LiteRGSS::Window` — matches litergss2 PSDK API. The
    // host-window equivalent moved to `LiteRGSS::DisplayWindow` in this port.
    rb_cFramedView = rb_define_class_under(rb_mLiteRGSS, "Window", rb_cDrawable);
    rb_define_alloc_func(rb_cFramedView, rb::Alloc<FramedViewData>);

    rb_define_method(rb_cFramedView, "initialize", _rbf rb_FramedView_Initialize, -1);
    rb_define_method(rb_cFramedView, "dispose", _rbf rb_FramedView_Dispose, 0);
    rb_define_method(rb_cFramedView, "disposed?", _rbf rb_FramedView_Disposed, 0);
    rb_define_method(rb_cFramedView, "update", _rbf rb_FramedView_Update, 0);
    rb_define_method(rb_cFramedView, "sort_z", _rbf rb_FramedView_SortZ, 0);

    rb_define_method(rb_cFramedView, "windowskin", _rbf rb_FramedView_get_skin, 0);
    rb_define_method(rb_cFramedView, "windowskin=", _rbf rb_FramedView_set_skin, 1);
    rb_define_method(rb_cFramedView, "width", _rbf rb_FramedView_get_width, 0);
    rb_define_method(rb_cFramedView, "width=", _rbf rb_FramedView_set_width, 1);
    rb_define_method(rb_cFramedView, "height", _rbf rb_FramedView_get_height, 0);
    rb_define_method(rb_cFramedView, "height=", _rbf rb_FramedView_set_height, 1);
    rb_define_method(rb_cFramedView, "set_size", _rbf rb_FramedView_setSize, 2);
    rb_define_method(rb_cFramedView, "window_builder", _rbf rb_FramedView_get_window_builder, 0);
    rb_define_method(rb_cFramedView, "window_builder=", _rbf rb_FramedView_set_window_builder, 1);
    rb_define_method(rb_cFramedView, "x", _rbf rb_FramedView_get_x, 0);
    rb_define_method(rb_cFramedView, "x=", _rbf rb_FramedView_set_x, 1);
    rb_define_method(rb_cFramedView, "y", _rbf rb_FramedView_get_y, 0);
    rb_define_method(rb_cFramedView, "y=", _rbf rb_FramedView_set_y, 1);
    rb_define_method(rb_cFramedView, "set_position", _rbf rb_FramedView_setPosition, 2);
    rb_define_method(rb_cFramedView, "z", _rbf rb_FramedView_get_z, 0);
    rb_define_method(rb_cFramedView, "z=", _rbf rb_FramedView_set_z, 1);
    rb_define_method(rb_cFramedView, "ox", _rbf rb_FramedView_get_ox, 0);
    rb_define_method(rb_cFramedView, "ox=", _rbf rb_FramedView_set_ox, 1);
    rb_define_method(rb_cFramedView, "oy", _rbf rb_FramedView_get_oy, 0);
    rb_define_method(rb_cFramedView, "oy=", _rbf rb_FramedView_set_oy, 1);
    rb_define_method(rb_cFramedView, "set_origin", _rbf rb_FramedView_setOrigin, 2);
    rb_define_method(rb_cFramedView, "cursor_rect", _rbf rb_FramedView_get_cursor_rect, 0);
    rb_define_method(rb_cFramedView, "cursor_rect=", _rbf rb_FramedView_set_cursor_rect, 1);
    rb_define_method(rb_cFramedView, "cursorskin", _rbf rb_FramedView_get_cursorskin, 0);
    rb_define_method(rb_cFramedView, "cursorskin=", _rbf rb_FramedView_set_cursorskin, 1);
    rb_define_method(rb_cFramedView, "pauseskin", _rbf rb_FramedView_get_pauseskin, 0);
    rb_define_method(rb_cFramedView, "pauseskin=", _rbf rb_FramedView_set_pauseskin, 1);
    rb_define_method(rb_cFramedView, "pause", _rbf rb_FramedView_get_pause, 0);
    rb_define_method(rb_cFramedView, "pause=", _rbf rb_FramedView_set_pause, 1);
    rb_define_method(rb_cFramedView, "pause_x", _rbf rb_FramedView_get_pause_x, 0);
    rb_define_method(rb_cFramedView, "pause_x=", _rbf rb_FramedView_set_pause_x, 1);
    rb_define_method(rb_cFramedView, "pause_y", _rbf rb_FramedView_get_pause_y, 0);
    rb_define_method(rb_cFramedView, "pause_y=", _rbf rb_FramedView_set_pause_y, 1);
    rb_define_method(rb_cFramedView, "active", _rbf rb_FramedView_get_active, 0);
    rb_define_method(rb_cFramedView, "active=", _rbf rb_FramedView_set_active, 1);
    rb_define_method(rb_cFramedView, "stretch", _rbf rb_FramedView_get_stretch, 0);
    rb_define_method(rb_cFramedView, "stretch=", _rbf rb_FramedView_set_stretch, 1);
    rb_define_method(rb_cFramedView, "opacity", _rbf rb_FramedView_get_opacity, 0);
    rb_define_method(rb_cFramedView, "opacity=", _rbf rb_FramedView_set_opacity, 1);
    rb_define_method(rb_cFramedView, "back_opacity", _rbf rb_FramedView_get_back_opacity, 0);
    rb_define_method(rb_cFramedView, "back_opacity=", _rbf rb_FramedView_set_back_opacity, 1);
    rb_define_method(rb_cFramedView, "contents_opacity", _rbf rb_FramedView_get_contents_opacity, 0);
    rb_define_method(rb_cFramedView, "contents_opacity=", _rbf rb_FramedView_set_contents_opacity, 1);
    rb_define_method(rb_cFramedView, "rect", _rbf rb_FramedView_rect, 0);
    rb_define_method(rb_cFramedView, "viewport", _rbf rb_FramedView_viewport, 0);
    rb_define_method(rb_cFramedView, "visible", _rbf rb_FramedView_get_visible, 0);
    rb_define_method(rb_cFramedView, "visible=", _rbf rb_FramedView_set_visible, 1);
    rb_define_method(rb_cFramedView, "__index__", _rbf rb_FramedView_index, 0);
    rb_define_method(rb_cFramedView, "lock", _rbf rb_FramedView_lock, 0);
    rb_define_method(rb_cFramedView, "unlock", _rbf rb_FramedView_unlock, 0);
    rb_define_method(rb_cFramedView, "locked?", _rbf rb_FramedView_locked, 0);
}
