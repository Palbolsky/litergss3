// LiteRGSS::Window — Ruby binding around cgss::FramedView. The native
// FramedView is registered into the parent (Viewport or DisplayWindow)
// at construction time and renders the 9-slice skin + cursor + pause
// indicator automatically inside cgss::DisplayWindow::draw().

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Views/DisplayWindow.h>
#include <LiteCGSS/Views/Viewport.h>
#include <array>
#include "FramedView.h"
#include "DisplayWindow.h"
#include "../graphics/Image.h"
#include "../graphics/Rect.h"
#include "../graphics/Viewport.h"
#include "../graphics/DrawableDisposable.h"

VALUE rb_cFramedView = Qnil;

namespace rb {
    template <>
    void Mark<FramedViewData>(void *ptr)
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

FramedViewData *get_framed_view(VALUE self) { return rb::GetPtr<FramedViewData>(self); }

// LiteRGSS2 ctor: Window.new(viewport_or_display_window). The arg is
// optional in litergss3 — when absent we attach to the active display
// window so old PSDK code that does `Window.new` keeps working.
VALUE rb_FramedView_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE parent;
    rb_scan_args(argc, argv, "01", &parent);

    auto *f = get_framed_view(self);
    auto *window = get_active_display_window();
    if (window == nullptr) {
        rb_raise(rb_eRGSSError, "Window.new requires an open DisplayWindow");
    }

    const bool is_viewport = RTEST(parent) && rb_obj_is_kind_of(parent, rb_cViewport) == Qtrue;
    if (is_viewport) {
        auto *vp = get_viewport(parent);
        if (vp == nullptr || !vp->viewport) {
            rb_raise(rb_eRGSSError, "Window.new viewport is not initialized");
        }
        f->view = std::make_unique<cgss::FramedView>(
            vp->viewport->addView<cgss::FramedView>(vp->viewport->weak()));
        f->rViewport = parent;
    } else {
        f->view = std::make_unique<cgss::FramedView>(
            window->addView<cgss::FramedView>());
        f->rViewport = Qnil;
    }
    f->has_view = true;
    return self;
}

VALUE rb_FramedView_Dispose(VALUE self)
{
    auto *f = get_framed_view(self);
    if (!f->disposed && f->has_view) {
        f->view->detach();
    }
    f->disposed = true;
    return self;
}

VALUE rb_FramedView_Disposed(VALUE self) { return get_framed_view(self)->disposed ? Qtrue : Qfalse; }

VALUE rb_FramedView_Update(VALUE self)
{
    auto *f = get_framed_view(self);
    if (f->has_view && !f->disposed) f->view->update();
    return self;
}

VALUE rb_FramedView_SortZ(VALUE self)
{
    auto *f = get_framed_view(self);
    if (f->has_view && !f->disposed) f->view->sortZ();
    return self;
}

// --- position / size / origin -----------------------------------------------

VALUE rb_FramedView_get_x(VALUE self) { return INT2NUM(get_framed_view(self)->x); }
VALUE rb_FramedView_set_x(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->x = NUM2INT(val);
    if (f->has_view) f->view->move(static_cast<float>(f->x), static_cast<float>(f->y));
    return val;
}
VALUE rb_FramedView_get_y(VALUE self) { return INT2NUM(get_framed_view(self)->y); }
VALUE rb_FramedView_set_y(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->y = NUM2INT(val);
    if (f->has_view) f->view->move(static_cast<float>(f->x), static_cast<float>(f->y));
    return val;
}
VALUE rb_FramedView_setPosition(VALUE self, VALUE x, VALUE y)
{
    auto *f = get_framed_view(self);
    f->x = NUM2INT(x); f->y = NUM2INT(y);
    if (f->has_view) f->view->move(static_cast<float>(f->x), static_cast<float>(f->y));
    return self;
}

VALUE rb_FramedView_get_width(VALUE self) { return INT2NUM(get_framed_view(self)->width); }
VALUE rb_FramedView_set_width(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->width = NUM2INT(val);
    if (f->has_view) f->view->resize(f->width, f->height);
    return val;
}
VALUE rb_FramedView_get_height(VALUE self) { return INT2NUM(get_framed_view(self)->height); }
VALUE rb_FramedView_set_height(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->height = NUM2INT(val);
    if (f->has_view) f->view->resize(f->width, f->height);
    return val;
}
VALUE rb_FramedView_setSize(VALUE self, VALUE w, VALUE h)
{
    auto *f = get_framed_view(self);
    f->width = NUM2INT(w); f->height = NUM2INT(h);
    if (f->has_view) f->view->resize(f->width, f->height);
    return self;
}

VALUE rb_FramedView_get_ox(VALUE self) { return INT2NUM(get_framed_view(self)->ox); }
VALUE rb_FramedView_set_ox(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->ox = NUM2INT(val);
    if (f->has_view) f->view->moveOrigin(f->ox, f->oy);
    return val;
}
VALUE rb_FramedView_get_oy(VALUE self) { return INT2NUM(get_framed_view(self)->oy); }
VALUE rb_FramedView_set_oy(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->oy = NUM2INT(val);
    if (f->has_view) f->view->moveOrigin(f->ox, f->oy);
    return val;
}
VALUE rb_FramedView_setOrigin(VALUE self, VALUE x, VALUE y)
{
    auto *f = get_framed_view(self);
    f->ox = NUM2INT(x); f->oy = NUM2INT(y);
    if (f->has_view) f->view->moveOrigin(f->ox, f->oy);
    return self;
}

VALUE rb_FramedView_get_z(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? LONG2NUM(f->view->getZ().z) : INT2NUM(0);
}
VALUE rb_FramedView_set_z(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->setZ(NUM2LONG(val));
    return val;
}

// --- visibility / active / pause / lock / stretch ---------------------------

VALUE rb_FramedView_get_visible(VALUE self)
{
    auto *f = get_framed_view(self);
    return (f->has_view && f->view->isVisible()) ? Qtrue : Qfalse;
}
VALUE rb_FramedView_set_visible(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->setVisible(RTEST(val));
    return val;
}

VALUE rb_FramedView_get_active(VALUE self)
{
    auto *f = get_framed_view(self);
    return (f->has_view && f->view->isActive()) ? Qtrue : Qfalse;
}
VALUE rb_FramedView_set_active(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->active(RTEST(val));
    return val;
}

VALUE rb_FramedView_get_pause(VALUE self)
{
    auto *f = get_framed_view(self);
    return (f->has_view && f->view->isPaused()) ? Qtrue : Qfalse;
}
VALUE rb_FramedView_set_pause(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->pause(RTEST(val));
    return val;
}

VALUE rb_FramedView_get_pause_x(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? LONG2NUM(f->view->getPauseX()) : INT2NUM(0);
}
VALUE rb_FramedView_set_pause_x(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view && !NIL_P(val)) {
        f->view->setPausePosition(static_cast<float>(NUM2LONG(val)),
                                  static_cast<float>(f->view->getPauseY()));
    }
    return val;
}
VALUE rb_FramedView_get_pause_y(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? LONG2NUM(f->view->getPauseY()) : INT2NUM(0);
}
VALUE rb_FramedView_set_pause_y(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view && !NIL_P(val)) {
        f->view->setPausePosition(static_cast<float>(f->view->getPauseX()),
                                  static_cast<float>(NUM2LONG(val)));
    }
    return val;
}

VALUE rb_FramedView_get_stretch(VALUE self)
{
    auto *f = get_framed_view(self);
    return (f->has_view && f->view->isStretched()) ? Qtrue : Qfalse;
}
VALUE rb_FramedView_set_stretch(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->stretch(RTEST(val));
    return val;
}

VALUE rb_FramedView_lock(VALUE self)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->lock();
    return self;
}
VALUE rb_FramedView_unlock(VALUE self)
{
    auto *f = get_framed_view(self);
    if (f->has_view) f->view->unlock();
    return self;
}
VALUE rb_FramedView_locked(VALUE self)
{
    auto *f = get_framed_view(self);
    return (f->has_view && f->view->isLocked()) ? Qtrue : Qfalse;
}

// --- opacity ----------------------------------------------------------------

VALUE rb_FramedView_get_opacity(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? INT2NUM(f->view->getOpacity()) : INT2NUM(255);
}
VALUE rb_FramedView_set_opacity(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    auto v = static_cast<uint8_t>(cgss::normalize_long(NUM2LONG(val), 0, 255));
    if (f->has_view) f->view->setOpacity(v);
    return val;
}
VALUE rb_FramedView_get_back_opacity(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? INT2NUM(f->view->getBackOpacity()) : INT2NUM(255);
}
VALUE rb_FramedView_set_back_opacity(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    auto v = static_cast<uint8_t>(cgss::normalize_long(NUM2LONG(val), 0, 255));
    if (f->has_view) f->view->setBackOpacity(v);
    return val;
}
VALUE rb_FramedView_get_contents_opacity(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? INT2NUM(f->view->getContentsOpacity()) : INT2NUM(255);
}
VALUE rb_FramedView_set_contents_opacity(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    auto v = static_cast<uint8_t>(cgss::normalize_long(NUM2LONG(val), 0, 255));
    if (f->has_view) f->view->setContentsOpacity(v);
    return val;
}

// --- skin / cursor / pause skin / window builder ----------------------------

VALUE rb_FramedView_get_skin(VALUE self) { return get_framed_view(self)->rWindowskin; }
VALUE rb_FramedView_set_skin(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->rWindowskin = val;
    if (!f->has_view) return val;
    if (NIL_P(val)) {
        f->view->setSkin(nullptr);
        return val;
    }
    if (rb_obj_is_kind_of(val, rb_cImage) != Qtrue) {
        rb_raise(rb_eRGSSError, "Window#windowskin= expects a Bitmap (Image).");
    }
    auto *img = get_image(val);
    if (!img->valid()) {
        f->view->setSkin(nullptr);
        return val;
    }
    f->view->setSkin(&image_acquire_texture(img));
    return val;
}

VALUE rb_FramedView_get_cursorskin(VALUE self) { return get_framed_view(self)->rCursorskin; }
VALUE rb_FramedView_set_cursorskin(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->rCursorskin = val;
    if (!f->has_view || NIL_P(val)) return val;
    if (rb_obj_is_kind_of(val, rb_cImage) != Qtrue) {
        rb_raise(rb_eRGSSError, "Window#cursorskin= expects a Bitmap (Image).");
    }
    auto *img = get_image(val);
    if (img->valid()) {
        f->view->setCursorSkin(image_acquire_texture(img));
    }
    return val;
}

VALUE rb_FramedView_get_pauseskin(VALUE self) { return get_framed_view(self)->rPauseskin; }
VALUE rb_FramedView_set_pauseskin(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    f->rPauseskin = val;
    if (!f->has_view || NIL_P(val)) return val;
    if (rb_obj_is_kind_of(val, rb_cImage) != Qtrue) {
        rb_raise(rb_eRGSSError, "Window#pauseskin= expects a Bitmap (Image).");
    }
    auto *img = get_image(val);
    if (img->valid()) {
        f->view->setPauseSkin(image_acquire_texture(img));
    }
    return val;
}

VALUE rb_FramedView_get_window_builder(VALUE self) { return get_framed_view(self)->rWindowBuilder; }
VALUE rb_FramedView_set_window_builder(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    Check_Type(val, T_ARRAY);
    const long len = RARRAY_LEN(val);
    if (len < 6) {
        rb_raise(rb_eArgError, "window_builder must have at least 6 integer entries.");
    }
    if (!f->has_view) {
        f->rWindowBuilder = val;
        return val;
    }
    if (len >= 8) {
        std::array<long, 8> wb{};
        for (long i = 0; i < 8; i++) wb[i] = NUM2LONG(rb_ary_entry(val, i));
        f->view->setBuilder(wb);
    } else {
        std::array<long, 6> wb{};
        for (long i = 0; i < 6; i++) wb[i] = NUM2LONG(rb_ary_entry(val, i));
        f->view->setBuilder(wb);
    }
    rb_obj_freeze(val);
    f->rWindowBuilder = val;
    return val;
}

// cursor_rect: PSDK reads/writes a Rect. We bind the cgss IntRect of the
// FramedView to a Ruby Rect's RectData on first read so subsequent
// `window.cursor_rect.set(x, y, w, h)` mutations land on the FramedView.
VALUE rb_FramedView_get_cursor_rect(VALUE self)
{
    auto *f = get_framed_view(self);
    if (!NIL_P(f->rCursorRect)) return f->rCursorRect;
    if (!f->has_view) {
        VALUE args[4] = { INT2NUM(0), INT2NUM(0), INT2NUM(0), INT2NUM(0) };
        f->rCursorRect = rb_class_new_instance(4, args, rb_cRect);
        return f->rCursorRect;
    }
    const auto rect = f->view->getCursorRectangle();
    VALUE args[4] = { INT2NUM(rect.left), INT2NUM(rect.top),
                      INT2NUM(rect.width), INT2NUM(rect.height) };
    f->rCursorRect = rb_class_new_instance(4, args, rb_cRect);
    return f->rCursorRect;
}

VALUE rb_FramedView_set_cursor_rect(VALUE self, VALUE val)
{
    auto *f = get_framed_view(self);
    if (!f->has_view) return val;
    int rx = 0, ry = 0, rw = 0, rh = 0;
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *r = get_rect_data(val);
        rx = r->x; ry = r->y; rw = r->width; rh = r->height;
    } else {
        Check_Type(val, T_ARRAY);
        rx = NUM2INT(rb_ary_entry(val, 0));
        ry = NUM2INT(rb_ary_entry(val, 1));
        rw = NUM2INT(rb_ary_entry(val, 2));
        rh = NUM2INT(rb_ary_entry(val, 3));
    }
    f->view->setCursorRectangle(cgss::IntRect{ rx, ry, rw, rh });
    // Cache a Ruby Rect so reads are stable.
    VALUE args[4] = { INT2NUM(rx), INT2NUM(ry), INT2NUM(rw), INT2NUM(rh) };
    f->rCursorRect = rb_class_new_instance(4, args, rb_cRect);
    return val;
}

// --- queries ----------------------------------------------------------------

VALUE rb_FramedView_rect(VALUE self)
{
    auto *f = get_framed_view(self);
    VALUE args[4] = { INT2NUM(f->x), INT2NUM(f->y),
                      INT2NUM(f->width), INT2NUM(f->height) };
    return rb_class_new_instance(4, args, rb_cRect);
}

VALUE rb_FramedView_viewport(VALUE self) { return get_framed_view(self)->rViewport; }
VALUE rb_FramedView_index(VALUE self)
{
    auto *f = get_framed_view(self);
    return f->has_view ? ULONG2NUM(f->view->getZ().index) : INT2NUM(0);
}

void Init_FramedView()
{
    // Registers as `LiteRGSS::Window` — matches litergss2 PSDK API. The
    // host-window equivalent is `LiteRGSS::DisplayWindow` in this port.
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
