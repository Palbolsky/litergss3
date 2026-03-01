#include "RaylibWrapper.h"
#include "LiteRGSS.h"
#include "RubyValue.h"
#include "common/Normalize.h"
#include "Sprite.h"
#include "Image.h"
#include "Viewport.h"

VALUE rb_cSprite = Qnil;

// --- TypedData ---

static void sprite_free(void* ptr)
{
    auto* s = static_cast<SpriteData*>(ptr);

    if (s->texture)
    {
        raylib::UnloadTexture(*s->texture);
        delete s->texture;
    }

    delete s;
}

static void sprite_mark(void* ptr)
{
    auto* s = static_cast<SpriteData*>(ptr);
    if (s == nullptr) return;
    rb_gc_mark(s->rBitmap);
    rb_gc_mark(s->rViewport);
}

static const rb_data_type_t sprite_type = {
    "SpriteData",
    { sprite_mark, sprite_free, nullptr },
    nullptr, nullptr,
    RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE sprite_alloc(VALUE klass)
{
    auto* s = new SpriteData();
    return TypedData_Wrap_Struct(klass, &sprite_type, s);
}

static SpriteData* get_sprite(VALUE self)
{
    SpriteData* s;
    TypedData_Get_Struct(self, SpriteData, &sprite_type, s);
    return s;
}

static void check_disposed(SpriteData* s)
{
    if (s->disposed)
        rb_raise(rb_eRuntimeError, "Sprite is disposed");
}

// --- initialize(viewport=nil) ---

VALUE rb_Sprite_Initialize(int argc, VALUE* argv, VALUE self)
{
    VALUE viewport;
    rb_scan_args(argc, argv, "01", &viewport);
    auto* s = get_sprite(self);
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
    auto* s = get_sprite(self);

    s->rBitmap = val;

    if (NIL_P(val))
    {
        s->texture = nullptr;
        return val;
    }

    ImageData* img = get_image(val);

    if (!img->valid())
    {
        s->texture = nullptr;
        return val;
    }

    if (s->texture)
    {
        raylib::UnloadTexture(*s->texture);
        delete s->texture;
        s->texture = nullptr;
    }
    s->texture = new raylib::Texture2D(raylib::LoadTextureFromImage(img->image));

    s->src_x = 0;
    s->src_y = 0;
    s->src_width  = img->width();
    s->src_height = img->height();

    return val;
}

VALUE rb_Sprite_getX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->x); }
VALUE rb_Sprite_getY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->y); }

VALUE rb_Sprite_setX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->x = (float)NUM2DBL(v);
    return v;
}

VALUE rb_Sprite_setY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->y = (float)NUM2DBL(v);
    return v;
}

// --- z ---

VALUE rb_Sprite_getZ(VALUE self) { check_disposed(get_sprite(self)); return INT2NUM(get_sprite(self)->z); }

VALUE rb_Sprite_setZ(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->z = NUM2INT(v);
    return v;
}

// --- ox / oy ---

VALUE rb_Sprite_getOX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->ox); }
VALUE rb_Sprite_getOY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->oy); }

VALUE rb_Sprite_setOX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->ox = (float)NUM2DBL(v);
    return v;
}

VALUE rb_Sprite_setOY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->oy = (float)NUM2DBL(v);
    return v;
}

// --- visible ---

VALUE rb_Sprite_getVisible(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->visible ? Qtrue : Qfalse; }

VALUE rb_Sprite_setVisible(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->visible = RTEST(v);
    return v;
}

// --- angle ---

VALUE rb_Sprite_getAngle(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->angle); }

VALUE rb_Sprite_setAngle(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->angle = (float)NUM2DBL(v);
    return v;
}

// --- zoom_x / zoom_y / zoom= ---

VALUE rb_Sprite_getZoomX(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_x); }
VALUE rb_Sprite_getZoomY(VALUE self) { check_disposed(get_sprite(self)); return DBL2NUM(get_sprite(self)->zoom_y); }

VALUE rb_Sprite_setZoomX(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->zoom_x = (float)NUM2DBL(v);
    return v;
}

VALUE rb_Sprite_setZoomY(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->zoom_y = (float)NUM2DBL(v);
    return v;
}

VALUE rb_Sprite_setZoom(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    float z = (float)NUM2DBL(v);
    get_sprite(self)->zoom_x = z;
    get_sprite(self)->zoom_y = z;
    return v;
}

// --- opacity ---

VALUE rb_Sprite_getOpacity(VALUE self)
{
    check_disposed(get_sprite(self));
    return INT2NUM(get_sprite(self)->opacity);
}

VALUE rb_Sprite_setOpacity(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->opacity = (uint8_t)normalize_long(RB_NUM2LONG(v), 0, 255);
    return v;
}

// --- mirror ---

VALUE rb_Sprite_getMirror(VALUE self) { check_disposed(get_sprite(self)); return get_sprite(self)->mirror ? Qtrue : Qfalse; }

VALUE rb_Sprite_setMirror(VALUE self, VALUE v)
{
    check_disposed(get_sprite(self));
    get_sprite(self)->mirror = RTEST(v);
    return v;
}

// --- src_rect as [x, y, w, h] ---

VALUE rb_Sprite_getRect(VALUE self)
{
    check_disposed(get_sprite(self));
    auto* s = get_sprite(self);
    VALUE ary = rb_ary_new_capa(4);
    rb_ary_push(ary, INT2NUM(s->src_x));
    rb_ary_push(ary, INT2NUM(s->src_y));
    rb_ary_push(ary, INT2NUM(s->src_width));
    rb_ary_push(ary, INT2NUM(s->src_height));
    return ary;
}

VALUE rb_Sprite_setRect(VALUE self, VALUE val)
{
    check_disposed(get_sprite(self));
    Check_Type(val, T_ARRAY);
    auto* s = get_sprite(self);
    s->src_x      = NUM2INT(rb_ary_entry(val, 0));
    s->src_y      = NUM2INT(rb_ary_entry(val, 1));
    s->src_width  = NUM2INT(rb_ary_entry(val, 2));
    s->src_height = NUM2INT(rb_ary_entry(val, 3));
    return val;
}

// --- set_position / set_origin ---

VALUE rb_Sprite_setPosition(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_sprite(self));
    auto* s = get_sprite(self);
    s->x = (float)NUM2DBL(x);
    s->y = (float)NUM2DBL(y);
    return self;
}

VALUE rb_Sprite_setOrigin(VALUE self, VALUE x, VALUE y)
{
    check_disposed(get_sprite(self));
    auto* s = get_sprite(self);
    s->ox = (float)NUM2DBL(x);
    s->oy = (float)NUM2DBL(y);
    return self;
}

// --- viewport ---

VALUE rb_Sprite_getViewport(VALUE self)
{
    check_disposed(get_sprite(self));
    return get_sprite(self)->rViewport;
}

// --- draw (called from Ruby render loop) ---

VALUE rb_Sprite_draw(VALUE self)
{
    auto* s = get_sprite(self);
    if (s->rViewport != Qnil)
    {
        auto* vp = get_viewport(s->rViewport);
        if (vp->disposed || !vp->visible)
            return self;
    }

    raylib::Rectangle src = {
        (float)s->src_x,
        (float)s->src_y,
        (float)(s->mirror ? -s->src_width : s->src_width),
        (float)s->src_height
    };

    raylib::Rectangle dst = {
        s->x,
        s->y,
        (float)s->src_width  * s->zoom_x,
        (float)s->src_height * s->zoom_y
    };

    raylib::Vector2 origin = { s->ox * s->zoom_x, s->oy * s->zoom_y };

    raylib::Color tint = { 255, 255, 255, s->opacity };

    raylib::DrawTexturePro(*s->texture, src, dst, origin, s->angle, tint);
    return self;
}

void Init_Sprite()
{
    rb_cSprite = rb_define_class_under(rb_mLiteRGSS, "Sprite", rb_cObject);
    rb_define_alloc_func(rb_cSprite, sprite_alloc);

    rb_define_method(rb_cSprite, "initialize",   _rbf rb_Sprite_Initialize,  -1);
    rb_define_method(rb_cSprite, "dispose",      _rbf rb_Sprite_Dispose,      0);
    rb_define_method(rb_cSprite, "disposed?",    _rbf rb_Sprite_Disposed,     0);
    rb_define_method(rb_cSprite, "clone",        _rbf rb_Sprite_Copy,         0);
    rb_define_method(rb_cSprite, "dup",          _rbf rb_Sprite_Copy,         0);
    rb_define_method(rb_cSprite, "bitmap",       _rbf rb_Sprite_getBitmap,    0);
    rb_define_method(rb_cSprite, "bitmap=",      _rbf rb_Sprite_setBitmap,    1);
    rb_define_method(rb_cSprite, "x",            _rbf rb_Sprite_getX,         0);
    rb_define_method(rb_cSprite, "x=",           _rbf rb_Sprite_setX,         1);
    rb_define_method(rb_cSprite, "y",            _rbf rb_Sprite_getY,         0);
    rb_define_method(rb_cSprite, "y=",           _rbf rb_Sprite_setY,         1);
    rb_define_method(rb_cSprite, "z",            _rbf rb_Sprite_getZ,         0);
    rb_define_method(rb_cSprite, "z=",           _rbf rb_Sprite_setZ,         1);
    rb_define_method(rb_cSprite, "ox",           _rbf rb_Sprite_getOX,        0);
    rb_define_method(rb_cSprite, "ox=",          _rbf rb_Sprite_setOX,        1);
    rb_define_method(rb_cSprite, "oy",           _rbf rb_Sprite_getOY,        0);
    rb_define_method(rb_cSprite, "oy=",          _rbf rb_Sprite_setOY,        1);
    rb_define_method(rb_cSprite, "visible",      _rbf rb_Sprite_getVisible,   0);
    rb_define_method(rb_cSprite, "visible=",     _rbf rb_Sprite_setVisible,   1);
    rb_define_method(rb_cSprite, "angle",        _rbf rb_Sprite_getAngle,     0);
    rb_define_method(rb_cSprite, "angle=",       _rbf rb_Sprite_setAngle,     1);
    rb_define_method(rb_cSprite, "zoom_x",       _rbf rb_Sprite_getZoomX,     0);
    rb_define_method(rb_cSprite, "zoom_x=",      _rbf rb_Sprite_setZoomX,     1);
    rb_define_method(rb_cSprite, "zoom_y",       _rbf rb_Sprite_getZoomY,     0);
    rb_define_method(rb_cSprite, "zoom_y=",      _rbf rb_Sprite_setZoomY,     1);
    rb_define_method(rb_cSprite, "zoom=",        _rbf rb_Sprite_setZoom,      1);
    rb_define_method(rb_cSprite, "opacity",      _rbf rb_Sprite_getOpacity,   0);
    rb_define_method(rb_cSprite, "opacity=",     _rbf rb_Sprite_setOpacity,   1);
    rb_define_method(rb_cSprite, "src_rect",     _rbf rb_Sprite_getRect,      0);
    rb_define_method(rb_cSprite, "src_rect=",    _rbf rb_Sprite_setRect,      1);
    rb_define_method(rb_cSprite, "mirror",       _rbf rb_Sprite_getMirror,    0);
    rb_define_method(rb_cSprite, "mirror=",      _rbf rb_Sprite_setMirror,    1);
    rb_define_method(rb_cSprite, "set_position", _rbf rb_Sprite_setPosition,  2);
    rb_define_method(rb_cSprite, "set_origin",   _rbf rb_Sprite_setOrigin,    2);
    rb_define_method(rb_cSprite, "viewport",     _rbf rb_Sprite_getViewport,  0);
    rb_define_method(rb_cSprite, "draw",         _rbf rb_Sprite_draw,         0);
}
