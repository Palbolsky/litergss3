// Ruby Text binding. Wraps cgss::Text via a per-Text private DrawableStack
// (see header for rationale + limitation). Drawing flushes the private
// stack into the active window's render target.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Backend/ActiveBackend.h>
#include <LiteCGSS/Graphics/RenderTarget.h>
#include <string>
#include "Text.h"
#include "Color.h"
#include "DrawableDisposable.h"
#include "Viewport.h"
#include "Fonts.h"
#include "DisplayWindow.h"

namespace {
    using Backend = cgss::backend::ActiveBackend;
    using Ops = Backend::Ops;
}

VALUE rb_cText = Qnil;

namespace rb {
    template <>
    inline void Mark<TextData>(void *ptr)
    {
        auto *t = static_cast<TextData *>(ptr);
        if (t == nullptr) return;
        rb_gc_mark(t->rViewport);
        rb_gc_mark(t->rWidth);
        rb_gc_mark(t->rHeight);
        rb_gc_mark(t->rText);
        rb_gc_mark(t->rAlign);
        rb_gc_mark(t->rX);
        rb_gc_mark(t->rY);
    }
}

static TextData *get_text(VALUE self) { return rb::GetPtr<TextData>(self); }

static void check_disposed(TextData *t)
{
    if (t->disposed) rb_raise(rb_eRGSSError, "Text is disposed");
}

VALUE rb_Text_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE fontid, viewport, x, y, width, height, str, align, outlinesize, colorid, sizeid;
    rb_scan_args(argc, argv, "74", &fontid, &viewport, &x, &y, &width, &height, &str,
                 &align, &outlinesize, &colorid, &sizeid);

    auto *t = get_text(self);
    t->text = cgss::Text::create(t->stack);
    t->rViewport = viewport;

    rb_check_type(x, T_FIXNUM); t->rX = x;
    rb_check_type(y, T_FIXNUM); t->rY = y;
    rb_check_type(width, T_FIXNUM); t->rWidth = width;
    rb_check_type(height, T_FIXNUM); t->rHeight = height;

    t->text.move(static_cast<float>(NUM2LONG(x)), static_cast<float>(NUM2LONG(y)));
    t->text.resize(NUM2LONG(width), NUM2LONG(height));

    if (!NIL_P(align)) {
        long a = NUM2LONG(align);
        if (a >= 0 && a <= 2) {
            t->rAlign = align;
            t->text.setAlign(static_cast<uint8_t>(a));
        }
    }
    if (!NIL_P(outlinesize)) {
        t->text.setOutlineThickness(static_cast<float>(NUM2DBL(outlinesize)));
    }

    // Font lookup mirrors litergss2's helper. Fonts are indexed in g_fontTable
    // (see Fonts.cpp) — id 0 is the default. rb_Fonts_get_font raises on bad id.
    long font_id = NUM2LONG(fontid);
    t->text.setFont(rb_Fonts_get_font(static_cast<unsigned long>(font_id)));

    if (!NIL_P(str)) {
        rb_check_type(str, T_STRING);
        t->rText = str;
        t->text.setString(std::string{StringValueCStr(str)});
    }
    return self;
}

VALUE rb_Text_Dispose(VALUE self)
{
    auto *t = get_text(self);
    if (!t->disposed) {
        t->text.detach();
        t->disposed = true;
    }
    return self;
}

VALUE rb_Text_Disposed(VALUE self) { return get_text(self)->disposed ? Qtrue : Qfalse; }

VALUE rb_Text_Copy(VALUE self) { (void)self; rb_raise(rb_eRGSSError, "Texts cannot be cloned or duplicated."); return self; }

VALUE rb_Text_getOpacity(VALUE self) { check_disposed(get_text(self)); return INT2NUM(get_text(self)->text.getOpacity()); }
VALUE rb_Text_setOpacity(VALUE self, VALUE val)
{
    check_disposed(get_text(self));
    get_text(self)->text.setOpacity(static_cast<uint8_t>(cgss::normalize_long(NUM2LONG(val), 0, 255)));
    return val;
}

VALUE rb_Text_getRealWidth(VALUE self) { return INT2NUM(static_cast<long>(get_text(self)->text.getRealWidth())); }
VALUE rb_Text_getRealHeight(VALUE self) { return INT2NUM(static_cast<long>(get_text(self)->text.getRealHeight())); }

VALUE rb_Text_getDrawShadow(VALUE self) { return get_text(self)->text.getDrawShadow() ? Qtrue : Qfalse; }
VALUE rb_Text_setDrawShadow(VALUE self, VALUE val) { get_text(self)->text.setDrawShadow(RTEST(val)); return val; }

VALUE rb_Text_getNumChar(VALUE self) { return UINT2NUM(get_text(self)->text.getNumCharToDraw()); }
VALUE rb_Text_setNumChar(VALUE self, VALUE val) { get_text(self)->text.setNumCharToDraw(NUM2ULONG(val)); return val; }

static VALUE color_to_rb(cgss::Color col)
{
    VALUE argv[4] = { INT2NUM(col.r), INT2NUM(col.g), INT2NUM(col.b), INT2NUM(col.a) };
    return rb_class_new_instance(4, argv, rb_cColor);
}

VALUE rb_Text_getFillColor(VALUE self) { return color_to_rb(get_text(self)->text.getFillColor()); }
VALUE rb_Text_setFillColor(VALUE self, VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cColor) != Qtrue)
        rb_raise(rb_eRGSSError, "Texts require a Color as fill_color= argument.");
    auto *cd = get_color_data(val);
    get_text(self)->text.setFillColor(cgss::Color{cd->r, cd->g, cd->b, cd->a});
    return val;
}

VALUE rb_Text_getOutlineColor(VALUE self) { return color_to_rb(get_text(self)->text.getOutlineColor()); }
VALUE rb_Text_setOutlineColor(VALUE self, VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cColor) != Qtrue)
        rb_raise(rb_eRGSSError, "Texts require a Color as outline_color= argument.");
    auto *cd = get_color_data(val);
    get_text(self)->text.setOutlineColor(cgss::Color{cd->r, cd->g, cd->b, cd->a});
    return val;
}

VALUE rb_Text_getOutlineThickness(VALUE self) { return DBL2NUM(get_text(self)->text.getOutlineThickness()); }
VALUE rb_Text_setOutlineThickness(VALUE self, VALUE val)
{
    get_text(self)->text.setOutlineThickness(static_cast<float>(NUM2DBL(val)));
    return val;
}

VALUE rb_Text_getZ(VALUE self) { return LONG2NUM(get_text(self)->text.getZ().z); }
VALUE rb_Text_setZ(VALUE self, VALUE val) { get_text(self)->text.setZ(NUM2LONG(val)); return val; }
VALUE rb_Text_Index(VALUE self) { return UINT2NUM(get_text(self)->text.getZ().index); }

VALUE rb_Text_getViewport(VALUE self) { return get_text(self)->rViewport; }

VALUE rb_Text_getAlign(VALUE self) { return get_text(self)->rAlign; }
VALUE rb_Text_setAlign(VALUE self, VALUE val)
{
    long a = NUM2LONG(val);
    get_text(self)->text.setAlign(static_cast<uint8_t>(a));
    get_text(self)->rAlign = val;
    return val;
}

VALUE rb_Text_setPosition(VALUE self, VALUE x, VALUE y)
{
    auto *t = get_text(self);
    t->text.move(static_cast<float>(NUM2LONG(x)), static_cast<float>(NUM2LONG(y)));
    t->rX = x; t->rY = y;
    return self;
}

VALUE rb_Text_getX(VALUE self) { return get_text(self)->rX; }
VALUE rb_Text_getY(VALUE self) { return get_text(self)->rY; }
VALUE rb_Text_setX(VALUE self, VALUE val)
{
    auto *t = get_text(self);
    t->text.move(static_cast<float>(NUM2LONG(val)), t->text.getY());
    t->rX = val; return val;
}
VALUE rb_Text_setY(VALUE self, VALUE val)
{
    auto *t = get_text(self);
    t->text.move(t->text.getX(), static_cast<float>(NUM2LONG(val)));
    t->rY = val; return val;
}

VALUE rb_Text_getWidth(VALUE self) { return get_text(self)->rWidth; }
VALUE rb_Text_setWidth(VALUE self, VALUE val)
{
    auto *t = get_text(self);
    t->text.resize(NUM2LONG(val), t->text.getHeight());
    t->rWidth = val; return val;
}
VALUE rb_Text_getHeight(VALUE self) { return get_text(self)->rHeight; }
VALUE rb_Text_setHeight(VALUE self, VALUE val)
{
    auto *t = get_text(self);
    t->text.resize(t->text.getWidth(), NUM2LONG(val));
    t->rHeight = val; return val;
}

VALUE rb_Text_getSize(VALUE self) { return INT2NUM(get_text(self)->text.getCharacterSize()); }
VALUE rb_Text_setSize(VALUE self, VALUE val)
{
    get_text(self)->text.setCharacterSize(cgss::normalize_long(NUM2LONG(val), 1, 0xFFFF));
    return val;
}

VALUE rb_Text_setText(VALUE self, VALUE str)
{
    rb_check_type(str, T_STRING);
    auto *t = get_text(self);
    t->rText = str;
    t->text.setString(std::string{StringValueCStr(str)});
    return str;
}

VALUE rb_Text_getText(VALUE self) { return get_text(self)->rText; }

VALUE rb_Text_getVisible(VALUE self) { return get_text(self)->text.isVisible() ? Qtrue : Qfalse; }
VALUE rb_Text_setVisible(VALUE self, VALUE val) { get_text(self)->text.setVisible(RTEST(val)); return val; }

VALUE rb_Text_getTextWidth(VALUE self, VALUE val)
{
    rb_check_type(val, T_STRING);
    return UINT2NUM(get_text(self)->text.getTextWidth(std::string{StringValueCStr(val)}));
}

VALUE rb_Text_draw(VALUE self)
{
    auto *t = get_text(self);
    if (t->disposed || !t->text.isVisible()) return self;
    auto *window = get_active_native_window();
    if (window == nullptr) return self;
    auto &target = Ops::target_from_window(*window);
    cgss::RenderTarget rt{target};
    t->stack.drawFast(rt);
    return self;
}

void Init_Text()
{
    rb_cText = rb_define_class_under(rb_mLiteRGSS, "Text", rb_cDrawable);
    rb_define_alloc_func(rb_cText, rb::Alloc<TextData>);

    rb_define_method(rb_cText, "initialize", _rbf rb_Text_Initialize, -1);
    rb_define_method(rb_cText, "dispose", _rbf rb_Text_Dispose, 0);
    rb_define_method(rb_cText, "disposed?", _rbf rb_Text_Disposed, 0);
    rb_define_method(rb_cText, "set_position", _rbf rb_Text_setPosition, 2);
    rb_define_method(rb_cText, "x", _rbf rb_Text_getX, 0);
    rb_define_method(rb_cText, "x=", _rbf rb_Text_setX, 1);
    rb_define_method(rb_cText, "y", _rbf rb_Text_getY, 0);
    rb_define_method(rb_cText, "y=", _rbf rb_Text_setY, 1);
    rb_define_method(rb_cText, "width", _rbf rb_Text_getWidth, 0);
    rb_define_method(rb_cText, "width=", _rbf rb_Text_setWidth, 1);
    rb_define_method(rb_cText, "height", _rbf rb_Text_getHeight, 0);
    rb_define_method(rb_cText, "height=", _rbf rb_Text_setHeight, 1);
    rb_define_method(rb_cText, "fill_color", _rbf rb_Text_getFillColor, 0);
    rb_define_method(rb_cText, "fill_color=", _rbf rb_Text_setFillColor, 1);
    rb_define_method(rb_cText, "outline_color", _rbf rb_Text_getOutlineColor, 0);
    rb_define_method(rb_cText, "outline_color=", _rbf rb_Text_setOutlineColor, 1);
    rb_define_method(rb_cText, "outline_thickness", _rbf rb_Text_getOutlineThickness, 0);
    rb_define_method(rb_cText, "outline_thickness=", _rbf rb_Text_setOutlineThickness, 1);
    rb_define_method(rb_cText, "align", _rbf rb_Text_getAlign, 0);
    rb_define_method(rb_cText, "align=", _rbf rb_Text_setAlign, 1);
    rb_define_method(rb_cText, "size", _rbf rb_Text_getSize, 0);
    rb_define_method(rb_cText, "size=", _rbf rb_Text_setSize, 1);
    rb_define_method(rb_cText, "set_size", _rbf rb_Text_setSize, 1);
    rb_define_method(rb_cText, "text", _rbf rb_Text_getText, 0);
    rb_define_method(rb_cText, "text=", _rbf rb_Text_setText, 1);
    rb_define_method(rb_cText, "visible", _rbf rb_Text_getVisible, 0);
    rb_define_method(rb_cText, "visible=", _rbf rb_Text_setVisible, 1);
    rb_define_method(rb_cText, "draw_shadow", _rbf rb_Text_getDrawShadow, 0);
    rb_define_method(rb_cText, "draw_shadow=", _rbf rb_Text_setDrawShadow, 1);
    rb_define_method(rb_cText, "nchar_draw", _rbf rb_Text_getNumChar, 0);
    rb_define_method(rb_cText, "nchar_draw=", _rbf rb_Text_setNumChar, 1);
    rb_define_method(rb_cText, "real_width", _rbf rb_Text_getRealWidth, 0);
    rb_define_method(rb_cText, "real_height", _rbf rb_Text_getRealHeight, 0);
    rb_define_method(rb_cText, "opacity", _rbf rb_Text_getOpacity, 0);
    rb_define_method(rb_cText, "opacity=", _rbf rb_Text_setOpacity, 1);
    rb_define_method(rb_cText, "text_width", _rbf rb_Text_getTextWidth, 1);
    rb_define_method(rb_cText, "z", _rbf rb_Text_getZ, 0);
    rb_define_method(rb_cText, "z=", _rbf rb_Text_setZ, 1);
    rb_define_method(rb_cText, "__index__", _rbf rb_Text_Index, 0);
    rb_define_method(rb_cText, "viewport", _rbf rb_Text_getViewport, 0);
    rb_define_method(rb_cText, "draw", _rbf rb_Text_draw, 0);
    rb_define_method(rb_cText, "clone", _rbf rb_Text_Copy, 0);
    rb_define_method(rb_cText, "dup", _rbf rb_Text_Copy, 0);
}
