// Backend-agnostic Ruby Sprite binding. Texture storage uses cgss::Texture;
// immediate-mode drawing routes through Ops::draw_texture_pro on the
// active window's render target (raylib: current BeginDrawing context;
// SFML: sf::RenderWindow / sf::RenderTexture).

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "window/DisplayWindow.h"
#include <LiteCGSS/Backend/ActiveBackend.h>
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include "Sprite.h"
#include "Image.h"
#include "Rect.h"
#include "Viewport.h"
#include "DrawableDisposable.h"

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

// GC roots SpriteData holds across the Ruby heap. Must be declared before
// any instantiation of rb::GetDataType<SpriteData> so this specialization
// lands in the generic type descriptor.
namespace rb {
    template <>
    inline void Mark<SpriteData>(void *ptr)
    {
        auto *s = static_cast<SpriteData *>(ptr);
        if (s == nullptr) return;
        rb_gc_mark(s->rBitmap);
        rb_gc_mark(s->rViewport);
    }
}

VALUE rb_cSprite = Qnil;

static SpriteData *get_sprite(VALUE self) { return rb::GetPtr<SpriteData>(self); }

static void check_disposed(SpriteData *s)
{
    if (s->disposed)
        rb_raise(rb_eRuntimeError, "Sprite is disposed");
}

// --- initialize(viewport=nil) ---

VALUE rb_Sprite_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE viewport;
    rb_scan_args(argc, argv, "01", &viewport);
    auto *s = get_sprite(self);
    s->rViewport = RTEST(viewport) ? viewport : Qnil;
    return self;
}

// --- dispose ---

VALUE rb_Sprite_Dispose(VALUE self)
{
    get_sprite(self)->disposed = true;
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

    // Allocate a fresh GPU texture sized to the image + upload its pixels.
    // Replacing s->texture drops the old shared_ptr; its custom deleter
    // invokes Ops::texture_destroy to release the prior GPU backing.
    s->texture = cgss::Texture::create(img->image);
    s->has_texture = true;

    s->src_x = 0;
    s->src_y = 0;
    s->src_width = static_cast<int>(img->width());
    s->src_height = static_cast<int>(img->height());

    return val;
}

VALUE rb_Sprite_getX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->x); }
VALUE rb_Sprite_getY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->y); }
VALUE rb_Sprite_setX(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->x = (float)NUM2DBL(v); return v; }
VALUE rb_Sprite_setY(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->y = (float)NUM2DBL(v); return v; }

VALUE rb_Sprite_getZ(VALUE self) { check_disposed(get_sprite(self)); return INT2NUM(get_sprite(self)->z); }
VALUE rb_Sprite_setZ(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->z = NUM2INT(v); return v; }

VALUE rb_Sprite_getOX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->ox); }
VALUE rb_Sprite_getOY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->oy); }
VALUE rb_Sprite_setOX(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->ox = (float)NUM2DBL(v); return v; }
VALUE rb_Sprite_setOY(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->oy = (float)NUM2DBL(v); return v; }

VALUE rb_Sprite_getVisible(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->visible ? Qtrue : Qfalse; }
VALUE rb_Sprite_setVisible(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->visible = RTEST(v); return v; }

VALUE rb_Sprite_getAngle(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->angle); }
VALUE rb_Sprite_setAngle(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->angle = (float)NUM2DBL(v); return v; }

VALUE rb_Sprite_getZoomX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_x); }
VALUE rb_Sprite_getZoomY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_y); }
VALUE rb_Sprite_setZoomX(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->zoom_x = (float)NUM2DBL(v); return v; }
VALUE rb_Sprite_setZoomY(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->zoom_y = (float)NUM2DBL(v); return v; }

VALUE rb_Sprite_setZoom(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    const float z = (float)NUM2DBL(v);
    auto *s = get_sprite(self);
    s->zoom_x = z;
    s->zoom_y = z;
    return v;
}

VALUE rb_Sprite_getOpacity(VALUE self) { check_disposed(get_sprite(self)); return INT2NUM(get_sprite(self)->opacity); }
VALUE rb_Sprite_setOpacity(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->opacity = (uint8_t)cgss::normalize_long(RB_NUM2LONG(v), 0, 255);
    return v;
}

VALUE rb_Sprite_getMirror(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->mirror ? Qtrue : Qfalse; }
VALUE rb_Sprite_setMirror(VALUE self, VALUE v) { check_disposed(get_sprite(self)); get_sprite(self)->mirror = RTEST(v); return v; }

// LiteRGSS2 returned a Rect instance for src_rect, not an Array. Match
// that — PSDK reads `sprite.src_rect.set(x, y, w, h)` and `.width`.
VALUE rb_Sprite_getRect(VALUE self)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    VALUE args[4] = { INT2NUM(s->src_x), INT2NUM(s->src_y),
                      INT2NUM(s->src_width), INT2NUM(s->src_height) };
    return rb_class_new_instance(4, args, rb_cRect);
}

VALUE rb_Sprite_setRect(VALUE self, VALUE val)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *r = get_rect_data(val);
        s->src_x = r->x;
        s->src_y = r->y;
        s->src_width  = r->width;
        s->src_height = r->height;
        return val;
    }
    Check_Type(val, T_ARRAY);
    s->src_x = NUM2INT(rb_ary_entry(val, 0));
    s->src_y = NUM2INT(rb_ary_entry(val, 1));
    s->src_width = NUM2INT(rb_ary_entry(val, 2));
    s->src_height = NUM2INT(rb_ary_entry(val, 3));
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
    return self;
}

VALUE rb_Sprite_setOrigin(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_sprite(self));
    auto *s = get_sprite(self);
    s->ox = (float)NUM2DBL(x);
    s->oy = (float)NUM2DBL(y);
    return self;
}

VALUE rb_Sprite_getViewport(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->rViewport; }

// --- draw (called from Ruby render loop) ---

VALUE rb_Sprite_draw(VALUE self)
{
    auto *s = get_sprite(self);
    if (s->disposed || !s->visible || !s->has_texture) return self;

    if (s->rViewport != Qnil) {
        auto *vp = get_viewport(s->rViewport);
        if (vp->disposed || !vp->visible) return self;
    }

    auto* window = get_active_native_window();
    if (window == nullptr) return self;

    // Route through the extension SPI to reach the native texture inside
    // the cgss::Texture wrapper. Ops::texture_share returns a shared_ptr;
    // we use it for the live native reference.
    auto nativePtr = Ops::texture_share(s->texture);
    if (!nativePtr) return self;

    auto& target = Ops::target_from_window(*window);

    const cgss::IntRect src{
        s->src_x, s->src_y,
        s->mirror ? -s->src_width : s->src_width,
        s->src_height };

    Ops::draw_texture_pro(
        target,
        *nativePtr,
        src,
        s->x, s->y,
        static_cast<float>(s->src_width)  * s->zoom_x,
        static_cast<float>(s->src_height) * s->zoom_y,
        s->ox * s->zoom_x,
        s->oy * s->zoom_y,
        s->angle,
        cgss::Color{ 255, 255, 255, s->opacity });

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
