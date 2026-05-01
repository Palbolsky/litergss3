// Ruby Shader binding. Subclass of BlendMode — both wrap the same
// cgss::RenderStates element. Loads GLSL via cgss::RenderStatesData and
// routes uniform sets through the backend-agnostic Glsl::* wrapper types.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Graphics/Glsl.h>
#include <LiteCGSS/Graphics/ShaderType.h>
#include <vector>
#include "Shader.h"
#include "Color.h"
#include "Tone.h"

VALUE rb_cShader = Qnil;

VALUE rb_Shader_loadFromMemory(int argc, VALUE *argv, VALUE self)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    VALUE arg1, arg2, arg3;
    rb_scan_args(argc, argv, "12", &arg1, &arg2, &arg3);
    rb_check_type(arg1, T_STRING);

    if (RTEST(arg2)) {
        if (rb_obj_is_kind_of(arg2, rb_cInteger) == Qtrue) {
            rs.data().loadShaderFromMemory(StringValueCStr(arg1),
                                           static_cast<cgss::ShaderType>(NUM2LONG(arg2)));
        } else {
            rb_check_type(arg2, T_STRING);
            if (argc == 2) {
                rs.data().loadShaderFromMemory(StringValueCStr(arg1), StringValueCStr(arg2));
            } else if (argc == 3) {
                rb_check_type(arg3, T_STRING);
                rs.data().loadShaderFromMemory(StringValueCStr(arg1),
                                               StringValueCStr(arg2),
                                               StringValueCStr(arg3));
            }
        }
    } else {
        rs.data().loadShaderFromMemory(StringValueCStr(arg1), cgss::ShaderType::Fragment);
    }
    return self;
}

VALUE rb_Shader_setFloatUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    const char *uname = StringValueCStr(name);

    if (rb_obj_is_kind_of(uniform, rb_cArray) == Qtrue) {
        long len = RARRAY_LEN(uniform);
        if (len == 4) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Vec4{
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 0))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 1))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 2))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 3)))});
        } else if (len == 3) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Vec3{
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 0))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 1))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 2)))});
        } else if (len == 2) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Vec2{
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 0))),
                static_cast<float>(NUM2DBL(rb_ary_entry(uniform, 1)))});
        }
    } else if (rb_obj_is_kind_of(uniform, rb_cColor) == Qtrue) {
        auto *cd = get_color_data(uniform);
        rs.data().setShaderUniform(uname, cgss::Glsl::Vec4{
            cd->r / 255.0f, cd->g / 255.0f, cd->b / 255.0f, cd->a / 255.0f});
    } else if (rb_obj_is_kind_of(uniform, rb_cTone) == Qtrue) {
        auto *t = get_tone_data(uniform);
        rs.data().setShaderUniform(uname, cgss::Glsl::Vec4{t->r, t->g, t->b, t->gray});
    } else {
        rs.data().setShaderUniform(uname, static_cast<float>(NUM2DBL(uniform)));
    }
    return self;
}

VALUE rb_Shader_setIntUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    const char *uname = StringValueCStr(name);

    if (rb_obj_is_kind_of(uniform, rb_cArray) == Qtrue) {
        long len = RARRAY_LEN(uniform);
        if (len == 4) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Ivec4{
                NUM2INT(rb_ary_entry(uniform, 0)), NUM2INT(rb_ary_entry(uniform, 1)),
                NUM2INT(rb_ary_entry(uniform, 2)), NUM2INT(rb_ary_entry(uniform, 3))});
        } else if (len == 3) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Ivec3{
                NUM2INT(rb_ary_entry(uniform, 0)), NUM2INT(rb_ary_entry(uniform, 1)),
                NUM2INT(rb_ary_entry(uniform, 2))});
        } else if (len == 2) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Ivec2{
                NUM2INT(rb_ary_entry(uniform, 0)), NUM2INT(rb_ary_entry(uniform, 1))});
        }
    } else {
        rs.data().setShaderUniform(uname, static_cast<int>(NUM2LONG(uniform)));
    }
    return self;
}

VALUE rb_Shader_setBoolUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    const char *uname = StringValueCStr(name);

    if (rb_obj_is_kind_of(uniform, rb_cArray) == Qtrue) {
        long len = RARRAY_LEN(uniform);
        if (len == 4) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Bvec4{
                (bool)RTEST(rb_ary_entry(uniform, 0)), (bool)RTEST(rb_ary_entry(uniform, 1)),
                (bool)RTEST(rb_ary_entry(uniform, 2)), (bool)RTEST(rb_ary_entry(uniform, 3))});
        } else if (len == 3) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Bvec3{
                (bool)RTEST(rb_ary_entry(uniform, 0)), (bool)RTEST(rb_ary_entry(uniform, 1)),
                (bool)RTEST(rb_ary_entry(uniform, 2))});
        } else if (len == 2) {
            rs.data().setShaderUniform(uname, cgss::Glsl::Bvec2{
                (bool)RTEST(rb_ary_entry(uniform, 0)), (bool)RTEST(rb_ary_entry(uniform, 1))});
        }
    } else {
        rs.data().setShaderUniform(uname, (bool)RTEST(uniform));
    }
    return self;
}

VALUE rb_Shader_setTextureUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    // Bitmap is the LiteRGSS::Bitmap alias of LiteRGSS::Image. Texture
    // binding via a ready-made cgss::Texture isn't yet hooked up here —
    // the Image-to-Texture bridge is in Sprite#bitmap=. For now, fall back
    // to the CurrentTexture sampler (sane default for fullscreen pp).
    (void)uniform;
    rs.data().setShaderUniform(StringValueCStr(name), cgss::CurrentTexture);
    return self;
}

VALUE rb_Shader_setMatrixUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    rb_check_type(uniform, T_ARRAY);
    long len = RARRAY_LEN(uniform);
    if (len == 9) {
        cgss::Glsl::Mat3 m;
        for (long i = 0; i < 9; ++i) m.array[i] = static_cast<float>(NUM2DBL(rb_ary_entry(uniform, i)));
        rs.data().setShaderUniform(StringValueCStr(name), m);
    } else if (len == 16) {
        cgss::Glsl::Mat4 m;
        for (long i = 0; i < 16; ++i) m.array[i] = static_cast<float>(NUM2DBL(rb_ary_entry(uniform, i)));
        rs.data().setShaderUniform(StringValueCStr(name), m);
    }
    return self;
}

VALUE rb_Shader_setFloatArrayUniform(VALUE self, VALUE name, VALUE uniform)
{
    auto &rs = rb::Get<RenderStatesElement>(self);
    rb_check_type(name, T_STRING);
    rb_check_type(uniform, T_ARRAY);
    long len = RARRAY_LEN(uniform);
    std::vector<float> floats(static_cast<size_t>(len));
    for (long i = 0; i < len; ++i)
        floats[i] = static_cast<float>(NUM2DBL(rb_ary_entry(uniform, i)));
    rs.data().setShaderUniformArray(StringValueCStr(name), floats.data(), static_cast<size_t>(len));
    return self;
}

VALUE rb_Shader_isAvailable(VALUE self) { (void)self; return RenderStatesElement::areShadersEnabled() ? Qtrue : Qfalse; }
VALUE rb_Shader_setAvailable(VALUE self, VALUE val) { (void)self; RenderStatesElement::enableShaders(RTEST(val)); return val; }
VALUE rb_Shader_isGeometryAvailable(VALUE self) { (void)self; return RenderStatesElement::areGeometryShadersEnabled() ? Qtrue : Qfalse; }

// True when the active GL context accepts the SFML legacy fixed-pipeline
// GLSL contract (gl_FragColor / gl_TexCoord[0] / texture2D / gl_Color).
// PSDK uses this to drive PSDK_SHADER_FIXED_PIPELINE — set it from
// Shader.fixed_pipeline? at game-load time so the right shader source
// variant gets emitted for the underlying backend (SFML always true;
// raylib true iff built with GRAPHICS_API_OPENGL_21 or _11).
VALUE rb_Shader_supportsFixedPipeline(VALUE self) {
    (void)self;
    return RenderStatesElement::supportsFixedPipeline() ? Qtrue : Qfalse;
}

VALUE rb_Shader_Copy(VALUE self) { (void)self; rb_raise(rb_eRGSSError, "Shaders cannot be cloned or duplicated."); return self; }

void Init_Shader()
{
    rb_cShader = rb_define_class_under(rb_mLiteRGSS, "Shader", rb_cBlendMode);
    rb_define_alloc_func(rb_cShader, rb::Alloc<RenderStatesElement>);

    rb_define_method(rb_cShader, "initialize", _rbf rb_Shader_loadFromMemory, -1);
    rb_define_method(rb_cShader, "load", _rbf rb_Shader_loadFromMemory, -1);
    rb_define_method(rb_cShader, "set_float_uniform", _rbf rb_Shader_setFloatUniform, 2);
    rb_define_method(rb_cShader, "set_int_uniform", _rbf rb_Shader_setIntUniform, 2);
    rb_define_method(rb_cShader, "set_bool_uniform", _rbf rb_Shader_setBoolUniform, 2);
    rb_define_method(rb_cShader, "set_texture_uniform", _rbf rb_Shader_setTextureUniform, 2);
    rb_define_method(rb_cShader, "set_matrix_uniform", _rbf rb_Shader_setMatrixUniform, 2);
    rb_define_method(rb_cShader, "set_float_array_uniform", _rbf rb_Shader_setFloatArrayUniform, 2);
    rb_define_method(rb_cShader, "clone", _rbf rb_Shader_Copy, 0);
    rb_define_method(rb_cShader, "dup", _rbf rb_Shader_Copy, 0);

    rb_define_singleton_method(rb_cShader, "available?", _rbf rb_Shader_isAvailable, 0);
    rb_define_singleton_method(rb_cShader, "available=", _rbf rb_Shader_setAvailable, 1);
    rb_define_singleton_method(rb_cShader, "is_geometry_available?", _rbf rb_Shader_isGeometryAvailable, 0);
    rb_define_singleton_method(rb_cShader, "fixed_pipeline?", _rbf rb_Shader_supportsFixedPipeline, 0);

    rb_define_const(rb_cShader, "Fragment", LONG2FIX(static_cast<long>(cgss::ShaderType::Fragment)));
    rb_define_const(rb_cShader, "Vertex", LONG2FIX(static_cast<long>(cgss::ShaderType::Vertex)));
    rb_define_const(rb_cShader, "Geometry", LONG2FIX(static_cast<long>(cgss::ShaderType::Geometry)));
}
