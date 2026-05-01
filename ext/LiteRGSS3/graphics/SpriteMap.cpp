// Ruby SpriteMap binding. The cgss::SpriteMap registers into the parent
// View's DrawableStack at SpriteMap.new time; cgss::DisplayWindow::draw()
// renders it automatically. `set(index, bitmap, rect)` builds (and caches)
// a cgss::Texture from the Image and calls cgss::SpriteMap::setTile.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Views/DisplayWindow.h>
#include <LiteCGSS/Views/Viewport.h>
#include "SpriteMap.h"
#include "Image.h"
#include "Rect.h"
#include "DrawableDisposable.h"
#include "DisplayWindow.h"
#include "Viewport.h"

VALUE rb_cSpriteMap = Qnil;

namespace rb {
    template <>
    void Mark<SpriteMapData>(void *ptr)
    {
        auto *s = static_cast<SpriteMapData *>(ptr);
        if (s == nullptr) return;
        rb_gc_mark(s->rViewport);
        rb_gc_mark(s->rX);
        rb_gc_mark(s->rY);
        rb_gc_mark(s->rOX);
        rb_gc_mark(s->rOY);
        rb_gc_mark(s->rScale);
    }
}

static SpriteMapData *get_smap(VALUE self) { return rb::GetPtr<SpriteMapData>(self); }

VALUE rb_SpriteMap_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE viewport, tile_width, tile_count;
    rb_scan_args(argc, argv, "30", &viewport, &tile_width, &tile_count);
    auto *s = get_smap(self);
    s->rViewport = viewport;

    if (get_active_display_window() == nullptr) {
        rb_raise(rb_eRGSSError, "SpriteMap.new requires an open DisplayWindow");
    }
    // cgss authorizations only allow SpriteMap on a Viewport — DisplayWindow
    // refuses it (see ViewAuthorizations specializations).
    const bool is_viewport = RTEST(viewport) && rb_obj_is_kind_of(viewport, rb_cViewport) == Qtrue;
    if (!is_viewport) {
        rb_raise(rb_eRGSSError, "SpriteMap.new requires a Viewport as parent");
    }
    auto *vp = get_viewport(viewport);
    if (vp == nullptr || !vp->viewport) {
        rb_raise(rb_eRGSSError, "SpriteMap.new viewport is not initialized");
    }
    s->spriteMap = cgss::SpriteMap::create(*vp->viewport);
    s->spriteMap.defineMap(NUM2ULONG(tile_width), NUM2ULONG(tile_count));
    return self;
}

VALUE rb_SpriteMap_Dispose(VALUE self)
{
    auto *s = get_smap(self);
    if (!s->disposed) { s->spriteMap.detach(); s->disposed = true; }
    return self;
}
VALUE rb_SpriteMap_Disposed(VALUE self) { return get_smap(self)->disposed ? Qtrue : Qfalse; }
VALUE rb_SpriteMap_Copy(VALUE self) { (void)self; rb_raise(rb_eRGSSError, "SpriteMaps cannot be cloned or duplicated."); return self; }

VALUE rb_SpriteMap_Viewport(VALUE self) { return get_smap(self)->rViewport; }

VALUE rb_SpriteMap_X(VALUE self) { return get_smap(self)->rX; }
VALUE rb_SpriteMap_setX(VALUE self, VALUE val)
{
    auto *s = get_smap(self);
    s->spriteMap.move(static_cast<float>(NUM2DBL(val)), static_cast<float>(NUM2DBL(s->rY)));
    s->rX = val; return val;
}
VALUE rb_SpriteMap_Y(VALUE self) { return get_smap(self)->rY; }
VALUE rb_SpriteMap_setY(VALUE self, VALUE val)
{
    auto *s = get_smap(self);
    s->spriteMap.move(static_cast<float>(NUM2DBL(s->rX)), static_cast<float>(NUM2DBL(val)));
    s->rY = val; return val;
}
VALUE rb_SpriteMap_setPosition(VALUE self, VALUE x, VALUE y)
{
    auto *s = get_smap(self);
    s->spriteMap.move(static_cast<float>(NUM2DBL(x)), static_cast<float>(NUM2DBL(y)));
    s->rX = x; s->rY = y; return self;
}

VALUE rb_SpriteMap_Z(VALUE self) { return LONG2NUM(get_smap(self)->spriteMap.getZ().z); }
VALUE rb_SpriteMap_setZ(VALUE self, VALUE val) { get_smap(self)->spriteMap.setZ(NUM2LONG(val)); return val; }
VALUE rb_SpriteMap_Index(VALUE self) { return LONG2NUM(get_smap(self)->spriteMap.getZ().index); }

VALUE rb_SpriteMap_OX(VALUE self) { return get_smap(self)->rOX; }
VALUE rb_SpriteMap_setOX(VALUE self, VALUE val)
{
    auto *s = get_smap(self);
    s->spriteMap.moveOrigin(static_cast<float>(NUM2DBL(val)), static_cast<float>(NUM2DBL(s->rOY)));
    s->rOX = val; return val;
}
VALUE rb_SpriteMap_OY(VALUE self) { return get_smap(self)->rOY; }
VALUE rb_SpriteMap_setOY(VALUE self, VALUE val)
{
    auto *s = get_smap(self);
    s->spriteMap.moveOrigin(static_cast<float>(NUM2DBL(s->rOX)), static_cast<float>(NUM2DBL(val)));
    s->rOY = val; return val;
}
VALUE rb_SpriteMap_setOrigin(VALUE self, VALUE x, VALUE y)
{
    auto *s = get_smap(self);
    s->spriteMap.moveOrigin(static_cast<float>(NUM2DBL(x)), static_cast<float>(NUM2DBL(y)));
    s->rOX = x; s->rOY = y; return self;
}

VALUE rb_SpriteMap_Reset(VALUE self) { get_smap(self)->spriteMap.reset(); return self; }

// Helper: resolve an (x, y, w, h) tuple from either a LiteRGSS::Rect or a
// 4-element Array.
static cgss::IntRect rect_from_rb(VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cRect) == Qtrue) {
        const auto *r = get_rect_data(val);
        return cgss::IntRect{ r->x, r->y, r->width, r->height };
    }
    Check_Type(val, T_ARRAY);
    if (RARRAY_LEN(val) < 4) {
        rb_raise(rb_eArgError, "Rect array must have 4 elements");
    }
    return cgss::IntRect{
        NUM2INT(rb_ary_entry(val, 0)),
        NUM2INT(rb_ary_entry(val, 1)),
        NUM2INT(rb_ary_entry(val, 2)),
        NUM2INT(rb_ary_entry(val, 3)),
    };
}

// `set(index, bitmap, rect)` — uploads the Image to a GPU texture (cached
// per-bitmap-VALUE) and registers the tile at `index` cropped to `rect`.
VALUE rb_SpriteMap_Set(int argc, VALUE *argv, VALUE self)
{
    VALUE rIndex, rBitmap, rRect;
    rb_scan_args(argc, argv, "30", &rIndex, &rBitmap, &rRect);
    auto *s = get_smap(self);
    if (s->disposed) return self;

    if (NIL_P(rBitmap)) return self;
    ImageData *img = get_image(rBitmap);
    if (img == nullptr || !img->valid()) return self;

    // The Image owns its GPU texture (lazily uploaded). cgss::SpriteMap
    // takes a shared_ptr ref internally via texture_share, so the bitmap
    // can be GC'd later and the GPU handle stays alive — no per-binding
    // cache or rBitmap GC-rooting needed here.
    cgss::Texture& tex = image_acquire_texture(img);
    const auto rect = rect_from_rb(rRect);
    s->spriteMap.setTile(NUM2ULONG(rIndex), rect, tex);
    return self;
}

// `set_rect(index, rect)` — repositions an already-set tile's source rect
// without changing the bound texture.
VALUE rb_SpriteMap_SetRect(int argc, VALUE *argv, VALUE self)
{
    VALUE rIndex, rRect;
    rb_scan_args(argc, argv, "20", &rIndex, &rRect);
    auto *s = get_smap(self);
    if (s->disposed) return self;
    s->spriteMap.setTileRect(NUM2ULONG(rIndex), rect_from_rb(rRect));
    return self;
}

VALUE rb_SpriteMap_TileScale(VALUE self) { return get_smap(self)->rScale; }
VALUE rb_SpriteMap_setTileScale(VALUE self, VALUE val)
{
    get_smap(self)->spriteMap.setTileScale(static_cast<float>(NUM2DBL(val)));
    get_smap(self)->rScale = val; return val;
}

// Backwards-compat: cgss::DisplayWindow::draw() now iterates the parent
// View's DrawableStack and renders the SpriteMap automatically — kept as a
// no-op for callers that still issue per-frame `sprite_map.draw`.
VALUE rb_SpriteMap_draw(VALUE self)
{
    return self;
}

void Init_SpriteMap()
{
    rb_cSpriteMap = rb_define_class_under(rb_mLiteRGSS, "SpriteMap", rb_cDrawable);
    rb_define_alloc_func(rb_cSpriteMap, rb::Alloc<SpriteMapData>);

    rb_define_method(rb_cSpriteMap, "initialize", _rbf rb_SpriteMap_Initialize, -1);
    rb_define_method(rb_cSpriteMap, "dispose", _rbf rb_SpriteMap_Dispose, 0);
    rb_define_method(rb_cSpriteMap, "disposed?", _rbf rb_SpriteMap_Disposed, 0);
    rb_define_method(rb_cSpriteMap, "viewport", _rbf rb_SpriteMap_Viewport, 0);
    rb_define_method(rb_cSpriteMap, "x", _rbf rb_SpriteMap_X, 0);
    rb_define_method(rb_cSpriteMap, "x=", _rbf rb_SpriteMap_setX, 1);
    rb_define_method(rb_cSpriteMap, "y", _rbf rb_SpriteMap_Y, 0);
    rb_define_method(rb_cSpriteMap, "y=", _rbf rb_SpriteMap_setY, 1);
    rb_define_method(rb_cSpriteMap, "set_position", _rbf rb_SpriteMap_setPosition, 2);
    rb_define_method(rb_cSpriteMap, "z", _rbf rb_SpriteMap_Z, 0);
    rb_define_method(rb_cSpriteMap, "z=", _rbf rb_SpriteMap_setZ, 1);
    rb_define_method(rb_cSpriteMap, "ox", _rbf rb_SpriteMap_OX, 0);
    rb_define_method(rb_cSpriteMap, "ox=", _rbf rb_SpriteMap_setOX, 1);
    rb_define_method(rb_cSpriteMap, "oy", _rbf rb_SpriteMap_OY, 0);
    rb_define_method(rb_cSpriteMap, "oy=", _rbf rb_SpriteMap_setOY, 1);
    rb_define_method(rb_cSpriteMap, "set_origin", _rbf rb_SpriteMap_setOrigin, 2);
    rb_define_method(rb_cSpriteMap, "reset", _rbf rb_SpriteMap_Reset, 0);
    rb_define_method(rb_cSpriteMap, "set", _rbf rb_SpriteMap_Set, -1);
    rb_define_method(rb_cSpriteMap, "set_rect", _rbf rb_SpriteMap_SetRect, -1);
    rb_define_method(rb_cSpriteMap, "tile_scale", _rbf rb_SpriteMap_TileScale, 0);
    rb_define_method(rb_cSpriteMap, "tile_scale=", _rbf rb_SpriteMap_setTileScale, 1);
    rb_define_method(rb_cSpriteMap, "__index__", _rbf rb_SpriteMap_Index, 0);
    rb_define_method(rb_cSpriteMap, "draw", _rbf rb_SpriteMap_draw, 0);
    rb_define_method(rb_cSpriteMap, "clone", _rbf rb_SpriteMap_Copy, 0);
    rb_define_method(rb_cSpriteMap, "dup", _rbf rb_SpriteMap_Copy, 0);
}
