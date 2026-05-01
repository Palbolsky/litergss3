// Backend-agnostic Ruby Sprite binding. The cgss::Sprite is registered into
// the parent View's DrawableStack at Ruby Sprite.new time; draws happen
// automatically inside cgss::DisplayWindow::draw(). All setters forward to
// the cgss::Sprite proxy.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "window/DisplayWindow.h"
#include "Sprite.h"
#include "Image.h"
#include "Rect.h"
#include "Viewport.h"
#include "DrawableDisposable.h"

#include <LiteCGSS/Common/Color.h>
#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Views/DisplayWindow.h>
#include <LiteCGSS/Views/Viewport.h>

namespace rb {
    template <>
    void Mark<SpriteData>(void *ptr)
    {
        auto *s = static_cast<SpriteData *>(ptr);
        if (s == nullptr) return;
        rb_gc_mark(s->rBitmap);
        rb_gc_mark(s->rViewport);
        rb_gc_mark(s->rRect);
    }
}

VALUE rb_cSprite = Qnil;

static SpriteData *get_sprite(VALUE self) { return rb::GetPtr<SpriteData>(self); }

static void check_disposed(SpriteData *s)
{
    if (s->disposed)
        rb_raise(rb_eRuntimeError, "Sprite is disposed");
}

// Apply current opacity via setColor (cgss::Sprite has no direct opacity
// setter on the Drawable; the SpriteItem one isn't reachable without a
// weak_ptr lock — color carries the alpha channel for our usage).
static void apply_opacity(SpriteData *s)
{
    if (!s->has_sprite) return;
    s->sprite->setColor(cgss::Color{ 255, 255, 255, s->opacity });
}

static void apply_src_rect(SpriteData *s)
{
    if (!s->has_sprite || !s->has_texture) return;
    s->sprite->setTextureRect(cgss::IntRect{
        s->src_x, s->src_y, s->src_width, s->src_height });
}

// --- initialize(viewport=nil) ---

VALUE rb_Sprite_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE parent;
    rb_scan_args(argc, argv, "01", &parent);
    auto *s = get_sprite(self);

    // Register into the parent View. LiteRGSS2 accepted either a Viewport
    // (register in viewport's child stack) or the DisplayWindow itself
    // (register at window level). PSDK uses the latter for ShaderedSprite —
    // see Graphics.rb#freeze.
    auto* window = get_active_display_window();
    if (window == nullptr) {
        rb_raise(rb_eRGSSError, "Sprite.new requires an open DisplayWindow");
    }
    const bool is_viewport = RTEST(parent) && rb_obj_is_kind_of(parent, rb_cViewport) == Qtrue;
    s->rViewport = is_viewport ? parent : Qnil;

    if (is_viewport) {
        auto *vp = get_viewport(parent);
        if (vp == nullptr || !vp->viewport) {
            rb_raise(rb_eRGSSError, "Sprite.new viewport is not initialized");
        }
        s->sprite = std::make_unique<cgss::Sprite>(cgss::Sprite::create(*vp->viewport));
    } else {
        s->sprite = std::make_unique<cgss::Sprite>(cgss::Sprite::create(*window));
    }
    s->has_sprite = true;
    s->sprite->setVisible(s->visible);
    return self;
}

// --- dispose ---

VALUE rb_Sprite_Dispose(VALUE self)
{
    auto *s = get_sprite(self);
    if (!s->disposed && s->has_sprite) {
        s->sprite->detach();
    }
    s->disposed = true;
    return self;
}

VALUE rb_Sprite_Disposed(VALUE self)
{
    return get_sprite(self)->disposed ? Qtrue : Qfalse;
}

VALUE rb_Sprite_Copy(VALUE self)
{
    (void)self;
    rb_raise(rb_eRuntimeError, "Sprites cannot be cloned or duplicated.");
    return self;
}

// --- bitmap ---

VALUE rb_Sprite_getBitmap(VALUE self)
{
    check_disposed(get_sprite(self));
    return get_sprite(self)->rBitmap;
}

VALUE rb_Sprite_setBitmap(VALUE self, VALUE val)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);

    s->rBitmap = val;

    if (NIL_P(val)) {
        s->texture = cgss::Texture{};
        s->has_texture = false;
        return val;
    }

    ImageData *img = get_image(val);
    if (!img->valid()) {
        s->texture = cgss::Texture{};
        s->has_texture = false;
        return val;
    }

    // Reuse the GPU texture cached on the Image. Multiple Sprites bound to
    // the same Bitmap share one upload — and cgss::Sprite::setTexture takes
    // its own shared_ptr ref via texture_share, so even if the Image is
    // GC'd later the GPU handle stays alive for as long as this sprite
    // holds it. The local copy in s->texture deep-copies the CPU image
    // mirror but only on rebind, not per frame.
    s->texture = image_acquire_texture(img);
    s->has_texture = true;

    if (!s->src_rect_user_set) {
        s->src_x = 0;
        s->src_y = 0;
        s->src_width  = static_cast<int>(img->width());
        s->src_height = static_cast<int>(img->height());
        // Keep the cached Rect (if any) in sync so a later
        // `sprite.src_rect` read sees the new texture's full extent.
        if (!NIL_P(s->rRect)) {
            auto *r = get_rect_data(s->rRect);
            r->x      = s->src_x;
            r->y      = s->src_y;
            r->width  = s->src_width;
            r->height = s->src_height;
        }
    }

    if (s->has_sprite) {
        // resetRect=true seeds the native sprite's rect from the texture
        // size. We then re-apply the user's src_rect (Ruby-cached) and any
        // pending mirror flag — both require the texture to be bound first.
        s->sprite->setTexture(s->texture, true);
        apply_src_rect(s);
        if (s->mirror) s->sprite->setMirror(true);
        apply_opacity(s);
    }
    return val;
}

VALUE rb_Sprite_getX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->x); }
VALUE rb_Sprite_getY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->y); }
VALUE rb_Sprite_setX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->x = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->move(s->x, s->y);
    return v;
}
VALUE rb_Sprite_setY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->y = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->move(s->x, s->y);
    return v;
}

VALUE rb_Sprite_getZ(VALUE self) { check_disposed(get_sprite(self)); return INT2NUM(get_sprite(self)->z); }
VALUE rb_Sprite_setZ(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->z = NUM2INT(v);
    if (s->has_sprite) s->sprite->setZ(s->z);
    return v;
}

VALUE rb_Sprite_getOX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->ox); }
VALUE rb_Sprite_getOY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->oy); }
VALUE rb_Sprite_setOX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->ox = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->moveOrigin(s->ox, s->oy);
    return v;
}
VALUE rb_Sprite_setOY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->oy = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->moveOrigin(s->ox, s->oy);
    return v;
}

VALUE rb_Sprite_getVisible(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->visible ? Qtrue : Qfalse; }
VALUE rb_Sprite_setVisible(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->visible = RTEST(v);
    if (s->has_sprite) s->sprite->setVisible(s->visible);
    return v;
}

VALUE rb_Sprite_getAngle(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->angle); }
VALUE rb_Sprite_setAngle(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->angle = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->setAngle(s->angle);
    return v;
}

VALUE rb_Sprite_getZoomX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_x); }
VALUE rb_Sprite_getZoomY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_y); }
VALUE rb_Sprite_setZoomX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->zoom_x = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->scale(s->zoom_x, s->zoom_y);
    return v;
}
VALUE rb_Sprite_setZoomY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->zoom_y = (float)NUM2DBL(v);
    if (s->has_sprite) s->sprite->scale(s->zoom_x, s->zoom_y);
    return v;
}

VALUE rb_Sprite_setZoom(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    const float z = (float)NUM2DBL(v);
    auto *s = get_sprite(self);
    s->zoom_x = z;
    s->zoom_y = z;
    if (s->has_sprite) s->sprite->scale(z, z);
    return v;
}

VALUE rb_Sprite_getOpacity(VALUE self) { check_disposed(get_sprite(self)); return INT2NUM(get_sprite(self)->opacity); }
VALUE rb_Sprite_setOpacity(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->opacity = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    apply_opacity(s);
    return v;
}

VALUE rb_Sprite_getMirror(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->mirror ? Qtrue : Qfalse; }
VALUE rb_Sprite_setMirror(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    const bool wanted = RTEST(v);
    s->mirror = wanted;
    if (s->has_sprite && s->has_texture) {
        s->sprite->setMirror(wanted);
    }
    return v;
}

// Pulled from `s->rRect`'s RectData when the user mutates the cached Rect
// returned by `sprite.src_rect`. Reads the latest Rect values into the
// sprite's int fields and re-applies to the native sprite. Triggered by
// the Rect's on_change hook installed in ensure_src_rect_cache.
static void sprite_rect_did_change(VALUE sprite_value)
{
    auto *s = get_sprite(sprite_value);
    if (s == nullptr || NIL_P(s->rRect)) return;
    const auto *r = get_rect_data(s->rRect);
    s->src_x      = r->x;
    s->src_y      = r->y;
    s->src_width  = r->width;
    s->src_height = r->height;
    s->src_rect_user_set = true;
    apply_src_rect(s);
}

// Lazily allocate the Rect cached on the sprite, syncing its values from
// the sprite's current src_x/y/width/height and wiring its on_change hook
// back to this sprite. Subsequent calls return the same Rect instance, so
// `sprite.src_rect.set(0, 0, 32, 32)` mutates state the sprite actually
// reads.
static VALUE ensure_src_rect_cache(VALUE self)
{
    auto *s = get_sprite(self);
    if (NIL_P(s->rRect)) {
        VALUE args[4] = { INT2NUM(s->src_x), INT2NUM(s->src_y),
                          INT2NUM(s->src_width), INT2NUM(s->src_height) };
        s->rRect = rb_class_new_instance(4, args, rb_cRect);
        auto *r = get_rect_data(s->rRect);
        r->owner = self;
        r->on_change = sprite_rect_did_change;
    } else {
        // Already allocated — refresh its values from the sprite in case
        // anything (e.g. setBitmap with !src_rect_user_set) updated the
        // sprite's int fields directly without going through the Rect.
        auto *r = get_rect_data(s->rRect);
        r->x      = s->src_x;
        r->y      = s->src_y;
        r->width  = s->src_width;
        r->height = s->src_height;
    }
    return s->rRect;
}

// LiteRGSS2 returned a Rect instance for src_rect, not an Array. Match
// that — PSDK reads `sprite.src_rect.set(x, y, w, h)` and `.width`.
VALUE rb_Sprite_getRect(VALUE self)
{
    check_disposed(get_sprite(self));
    return ensure_src_rect_cache(self);
}

VALUE rb_Sprite_setRect(VALUE self, VALUE val)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    // Mutate the cached Rect (creating it if needed) so any prior reference
    // the user holds keeps observing the same Rect.
    VALUE rect = ensure_src_rect_cache(self);
    auto *dst = get_rect_data(rect);
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *src = get_rect_data(val);
        dst->x = src->x;
        dst->y = src->y;
        dst->width  = src->width;
        dst->height = src->height;
    } else {
        Check_Type(val, T_ARRAY);
        dst->x = NUM2INT(rb_ary_entry(val, 0));
        dst->y = NUM2INT(rb_ary_entry(val, 1));
        dst->width = NUM2INT(rb_ary_entry(val, 2));
        dst->height = NUM2INT(rb_ary_entry(val, 3));
    }
    sprite_rect_did_change(self);
    return val;
}

// LiteRGSS2 sprites had width/height that proxied to the bound bitmap.
// PSDK's Plane class and a few mouse-hover helpers rely on this.
VALUE rb_Sprite_getWidth(VALUE self)
{
    auto *s = get_sprite(self);
    if (s->disposed) return INT2NUM(0);
    if (NIL_P(s->rBitmap)) return INT2NUM(0);
    return rb_funcall(s->rBitmap, rb_intern("width"), 0);
}

VALUE rb_Sprite_getHeight(VALUE self)
{
    auto *s = get_sprite(self);
    if (s->disposed) return INT2NUM(0);
    if (NIL_P(s->rBitmap)) return INT2NUM(0);
    return rb_funcall(s->rBitmap, rb_intern("height"), 0);
}

VALUE rb_Sprite_setPosition(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->x = (float)NUM2DBL(x);
    s->y = (float)NUM2DBL(y);
    if (s->has_sprite) s->sprite->move(s->x, s->y);
    return self;
}

VALUE rb_Sprite_setOrigin(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->ox = (float)NUM2DBL(x);
    s->oy = (float)NUM2DBL(y);
    if (s->has_sprite) s->sprite->moveOrigin(s->ox, s->oy);
    return self;
}

VALUE rb_Sprite_getViewport(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->rViewport; }

// Backwards-compat: pre-Phase-2 the Ruby render loop drew sprites via
// explicit per-frame `sprite.draw` calls. cgss::DisplayWindow::draw() now
// iterates the DrawableStack and renders every registered sprite — this
// method is kept as a no-op so existing PSDK / test code keeps working.
VALUE rb_Sprite_draw(VALUE self)
{
    return self;
}

void Init_Sprite()
{
    rb_cSprite = rb_define_class_under(rb_mLiteRGSS, "Sprite", rb_cDrawable);
    rb_define_alloc_func(rb_cSprite, rb::Alloc<SpriteData>);

    rb_define_method(rb_cSprite, "initialize", _rbf rb_Sprite_Initialize, -1);
    rb_define_method(rb_cSprite, "dispose", _rbf rb_Sprite_Dispose, 0);
    rb_define_method(rb_cSprite, "disposed?", _rbf rb_Sprite_Disposed, 0);
    rb_define_method(rb_cSprite, "clone", _rbf rb_Sprite_Copy, 0);
    rb_define_method(rb_cSprite, "dup", _rbf rb_Sprite_Copy, 0);
    rb_define_method(rb_cSprite, "bitmap", _rbf rb_Sprite_getBitmap, 0);
    rb_define_method(rb_cSprite, "bitmap=", _rbf rb_Sprite_setBitmap, 1);
    rb_define_method(rb_cSprite, "x", _rbf rb_Sprite_getX, 0);
    rb_define_method(rb_cSprite, "x=", _rbf rb_Sprite_setX, 1);
    rb_define_method(rb_cSprite, "y", _rbf rb_Sprite_getY, 0);
    rb_define_method(rb_cSprite, "y=", _rbf rb_Sprite_setY, 1);
    rb_define_method(rb_cSprite, "z", _rbf rb_Sprite_getZ, 0);
    rb_define_method(rb_cSprite, "z=", _rbf rb_Sprite_setZ, 1);
    rb_define_method(rb_cSprite, "ox", _rbf rb_Sprite_getOX, 0);
    rb_define_method(rb_cSprite, "ox=", _rbf rb_Sprite_setOX, 1);
    rb_define_method(rb_cSprite, "oy", _rbf rb_Sprite_getOY, 0);
    rb_define_method(rb_cSprite, "oy=", _rbf rb_Sprite_setOY, 1);
    rb_define_method(rb_cSprite, "visible", _rbf rb_Sprite_getVisible, 0);
    rb_define_method(rb_cSprite, "visible=", _rbf rb_Sprite_setVisible, 1);
    rb_define_method(rb_cSprite, "angle", _rbf rb_Sprite_getAngle, 0);
    rb_define_method(rb_cSprite, "angle=", _rbf rb_Sprite_setAngle, 1);
    rb_define_method(rb_cSprite, "zoom_x", _rbf rb_Sprite_getZoomX, 0);
    rb_define_method(rb_cSprite, "zoom_x=", _rbf rb_Sprite_setZoomX, 1);
    rb_define_method(rb_cSprite, "zoom_y", _rbf rb_Sprite_getZoomY, 0);
    rb_define_method(rb_cSprite, "zoom_y=", _rbf rb_Sprite_setZoomY, 1);
    rb_define_method(rb_cSprite, "zoom=", _rbf rb_Sprite_setZoom, 1);
    rb_define_method(rb_cSprite, "opacity", _rbf rb_Sprite_getOpacity, 0);
    rb_define_method(rb_cSprite, "opacity=", _rbf rb_Sprite_setOpacity, 1);
    rb_define_method(rb_cSprite, "src_rect", _rbf rb_Sprite_getRect, 0);
    rb_define_method(rb_cSprite, "src_rect=", _rbf rb_Sprite_setRect, 1);
    rb_define_method(rb_cSprite, "width", _rbf rb_Sprite_getWidth, 0);
    rb_define_method(rb_cSprite, "height", _rbf rb_Sprite_getHeight, 0);
    rb_define_method(rb_cSprite, "mirror", _rbf rb_Sprite_getMirror, 0);
    rb_define_method(rb_cSprite, "mirror=", _rbf rb_Sprite_setMirror, 1);
    rb_define_method(rb_cSprite, "set_position", _rbf rb_Sprite_setPosition, 2);
    rb_define_method(rb_cSprite, "set_origin", _rbf rb_Sprite_setOrigin, 2);
    rb_define_method(rb_cSprite, "viewport", _rbf rb_Sprite_getViewport, 0);
    rb_define_method(rb_cSprite, "draw", _rbf rb_Sprite_draw, 0);
}
