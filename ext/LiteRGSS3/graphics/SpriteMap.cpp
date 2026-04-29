// Ruby SpriteMap binding. Wraps cgss::SpriteMap via per-SpriteMap private
// DrawableStack — same model as Text/Shape (see Text.h for rationale).
//
// Note on `set(index, bitmap, rect)`: cgss::SpriteMap::setTile takes a
// cgss::Texture&. Litergss3's Image class wraps cgss::Image (CPU pixels);
// to wire bitmap setting we'd need to construct/cache a cgss::Texture from
// the Image. That bridge is deferred — left as a stub that records the
// VALUE for GC tracking. The SpriteMap renders empty until then.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Backend/ActiveBackend.h>
#include <LiteCGSS/Graphics/RenderTarget.h>
#include "SpriteMap.h"
#include "DrawableDisposable.h"
#include "DisplayWindow.h"

namespace {
    using Backend = cgss::backend::ActiveBackend;
    using Ops = Backend::Ops;
}

VALUE rb_cSpriteMap = Qnil;

namespace rb {
    template <>
    inline void Mark<SpriteMapData>(void *ptr)
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
    s->spriteMap = cgss::SpriteMap::create(s->stack);
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

VALUE rb_SpriteMap_Set(int argc, VALUE *argv, VALUE self)
{
    // Texture binding deferred — see header note. Stub records args but
    // does not register tiles with cgss::SpriteMap (which would require a
    // cgss::Texture built from the Image's pixel data).
    (void)argc; (void)argv; (void)self;
    return self;
}

VALUE rb_SpriteMap_SetRect(int argc, VALUE *argv, VALUE self)
{
    (void)argc; (void)argv; (void)self;
    return self;
}

VALUE rb_SpriteMap_TileScale(VALUE self) { return get_smap(self)->rScale; }
VALUE rb_SpriteMap_setTileScale(VALUE self, VALUE val)
{
    get_smap(self)->spriteMap.setTileScale(static_cast<float>(NUM2DBL(val)));
    get_smap(self)->rScale = val; return val;
}

VALUE rb_SpriteMap_draw(VALUE self)
{
    auto *s = get_smap(self);
    if (s->disposed) return self;
    auto *window = get_active_native_window();
    if (window == nullptr) return self;
    auto &target = Ops::target_from_window(*window);
    cgss::RenderTarget rt{target};
    s->stack.drawFast(rt);
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
