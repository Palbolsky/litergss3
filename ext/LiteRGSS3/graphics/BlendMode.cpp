// Ruby BlendMode binding. Routes blend factor / equation changes through
// cgss::RenderStates::data() — fully backend-agnostic. Constants mirror
// cgss::BlendFactor / cgss::BlendEquation enum values, which themselves
// match sf::BlendMode::* numerically (see Graphics/BlendMode.h header).

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "BlendMode.h"

VALUE rb_cBlendMode = Qnil;

VALUE rb_BlendMode_setColorSrcFactor(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendColorSrcFactor(static_cast<cgss::BlendFactor>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getColorSrcFactor(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendColorSrcFactor()));
}

VALUE rb_BlendMode_setColorDestFactor(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendColorDstFactor(static_cast<cgss::BlendFactor>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getColorDestFactor(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendColorDstFactor()));
}

VALUE rb_BlendMode_setAlphaSrcFactor(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendAlphaSrcFactor(static_cast<cgss::BlendFactor>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getAlphaSrcFactor(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendAlphaSrcFactor()));
}

VALUE rb_BlendMode_setAlphaDestFactor(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendAlphaDstFactor(static_cast<cgss::BlendFactor>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getAlphaDestFactor(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendAlphaDstFactor()));
}

VALUE rb_BlendMode_setColorEquation(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendColorEquation(static_cast<cgss::BlendEquation>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getColorEquation(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendColorEquation()));
}

VALUE rb_BlendMode_setAlphaEquation(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rs.data().setBlendAlphaEquation(static_cast<cgss::BlendEquation>(NUM2LONG(val)));
    return val;
}
VALUE rb_BlendMode_getAlphaEquation(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    return LONG2NUM(static_cast<long>(rs.data().getBlendAlphaEquation()));
}

// 0 = normal, 1 = additive, 2 = subtractive.
// Litergss2 PSDK passes these integers directly; preserved verbatim.
VALUE rb_BlendMode_setBlendType(VALUE self, VALUE val)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    auto &d = rs.data();
    long mode = NUM2LONG(val);
    switch (mode) {
        case 1: // Add
            d.setBlendColorEquation(cgss::BlendEquation::Add);
            d.setBlendAlphaEquation(cgss::BlendEquation::Add);
            d.setBlendColorSrcFactor(cgss::BlendFactor::SrcAlpha);
            d.setBlendColorDstFactor(cgss::BlendFactor::One);
            d.setBlendAlphaSrcFactor(cgss::BlendFactor::One);
            d.setBlendAlphaDstFactor(cgss::BlendFactor::One);
            break;
        case 2: // Subtract
            d.setBlendColorEquation(cgss::BlendEquation::ReverseSubtract);
            d.setBlendAlphaEquation(cgss::BlendEquation::ReverseSubtract);
            d.setBlendColorSrcFactor(cgss::BlendFactor::SrcAlpha);
            d.setBlendColorDstFactor(cgss::BlendFactor::One);
            d.setBlendAlphaSrcFactor(cgss::BlendFactor::Zero);
            d.setBlendAlphaDstFactor(cgss::BlendFactor::One);
            break;
        default: // Normal
            d.setBlendColorEquation(cgss::BlendEquation::Add);
            d.setBlendAlphaEquation(cgss::BlendEquation::Add);
            d.setBlendColorSrcFactor(cgss::BlendFactor::SrcAlpha);
            d.setBlendColorDstFactor(cgss::BlendFactor::OneMinusSrcAlpha);
            d.setBlendAlphaSrcFactor(cgss::BlendFactor::One);
            d.setBlendAlphaDstFactor(cgss::BlendFactor::OneMinusSrcAlpha);
            break;
    }
    return val;
}

VALUE rb_BlendMode_getBlendType(VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    auto &d = rs.data();
    if (d.getBlendColorEquation() == cgss::BlendEquation::ReverseSubtract) return LONG2FIX(2);
    if (d.getBlendAlphaDstFactor() == cgss::BlendFactor::One) return LONG2FIX(1);
    return LONG2FIX(0);
}

VALUE rb_BlendMode_Copy(VALUE self)
{
    (void)self;
    rb_raise(rb_eRGSSError, "BlendMode cannot be cloned or duplicated.");
    return self;
}

void Init_BlendMode()
{
    rb_cBlendMode = rb_define_class_under(rb_mLiteRGSS, "BlendMode", rb_cObject);
    rb_define_alloc_func(rb_cBlendMode, rb::Alloc<RenderStatesElement>);

    rb_define_method(rb_cBlendMode, "color_src_factor", _rbf rb_BlendMode_getColorSrcFactor, 0);
    rb_define_method(rb_cBlendMode, "color_src_factor=", _rbf rb_BlendMode_setColorSrcFactor, 1);
    rb_define_method(rb_cBlendMode, "color_dest_factor", _rbf rb_BlendMode_getColorDestFactor, 0);
    rb_define_method(rb_cBlendMode, "color_dest_factor=", _rbf rb_BlendMode_setColorDestFactor, 1);
    rb_define_method(rb_cBlendMode, "alpha_src_factor", _rbf rb_BlendMode_getAlphaSrcFactor, 0);
    rb_define_method(rb_cBlendMode, "alpha_src_factor=", _rbf rb_BlendMode_setAlphaSrcFactor, 1);
    rb_define_method(rb_cBlendMode, "alpha_dest_factor", _rbf rb_BlendMode_getAlphaDestFactor, 0);
    rb_define_method(rb_cBlendMode, "alpha_dest_factor=", _rbf rb_BlendMode_setAlphaDestFactor, 1);
    rb_define_method(rb_cBlendMode, "color_equation", _rbf rb_BlendMode_getColorEquation, 0);
    rb_define_method(rb_cBlendMode, "color_equation=", _rbf rb_BlendMode_setColorEquation, 1);
    rb_define_method(rb_cBlendMode, "alpha_equation", _rbf rb_BlendMode_getAlphaEquation, 0);
    rb_define_method(rb_cBlendMode, "alpha_equation=", _rbf rb_BlendMode_setAlphaEquation, 1);
    rb_define_method(rb_cBlendMode, "blend_type", _rbf rb_BlendMode_getBlendType, 0);
    rb_define_method(rb_cBlendMode, "blend_type=", _rbf rb_BlendMode_setBlendType, 1);
    rb_define_method(rb_cBlendMode, "clone", _rbf rb_BlendMode_Copy, 0);
    rb_define_method(rb_cBlendMode, "dup", _rbf rb_BlendMode_Copy, 0);

    // Equation
    rb_define_const(rb_cBlendMode, "Add", LONG2FIX(static_cast<long>(cgss::BlendEquation::Add)));
    rb_define_const(rb_cBlendMode, "Subtract", LONG2FIX(static_cast<long>(cgss::BlendEquation::Subtract)));
    rb_define_const(rb_cBlendMode, "ReverseSubtract", LONG2FIX(static_cast<long>(cgss::BlendEquation::ReverseSubtract)));

    // Factor
    rb_define_const(rb_cBlendMode, "Zero", LONG2FIX(static_cast<long>(cgss::BlendFactor::Zero)));
    rb_define_const(rb_cBlendMode, "One", LONG2FIX(static_cast<long>(cgss::BlendFactor::One)));
    rb_define_const(rb_cBlendMode, "SrcColor", LONG2FIX(static_cast<long>(cgss::BlendFactor::SrcColor)));
    rb_define_const(rb_cBlendMode, "OneMinusSrcColor", LONG2FIX(static_cast<long>(cgss::BlendFactor::OneMinusSrcColor)));
    rb_define_const(rb_cBlendMode, "DstColor", LONG2FIX(static_cast<long>(cgss::BlendFactor::DstColor)));
    rb_define_const(rb_cBlendMode, "OneMinusDstColor", LONG2FIX(static_cast<long>(cgss::BlendFactor::OneMinusDstColor)));
    rb_define_const(rb_cBlendMode, "SrcAlpha", LONG2FIX(static_cast<long>(cgss::BlendFactor::SrcAlpha)));
    rb_define_const(rb_cBlendMode, "OneMinusSrcAlpha", LONG2FIX(static_cast<long>(cgss::BlendFactor::OneMinusSrcAlpha)));
    rb_define_const(rb_cBlendMode, "DstAlpha", LONG2FIX(static_cast<long>(cgss::BlendFactor::DstAlpha)));
    rb_define_const(rb_cBlendMode, "OneMinusDstAlpha", LONG2FIX(static_cast<long>(cgss::BlendFactor::OneMinusDstAlpha)));
}
