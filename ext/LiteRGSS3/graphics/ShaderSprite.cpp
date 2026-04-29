// Ruby ShaderedSprite binding. Subclass of Sprite that adds a shader /
// blendmode accessor. Storage reuses SpriteData (allocator is identical) —
// the shader binding lives in a Ruby instance variable @shader so Ruby's
// GC handles its lifetime without a separate C-side mark hook.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include "ShaderSprite.h"
#include "Sprite.h"
#include "BlendMode.h"

VALUE rb_cShaderSprite = Qnil;

static VALUE rb_ShaderSprite_getShader(VALUE self)
{
    return rb_iv_get(self, "@shader");
}

static VALUE rb_ShaderSprite_setShader(VALUE self, VALUE shader)
{
    if (!NIL_P(shader) && rb_obj_is_kind_of(shader, rb_cBlendMode) != Qtrue)
        rb_raise(rb_eRGSSError, "ShaderedSprite shader must be a BlendMode (or subclass).");
    rb_iv_set(self, "@shader", shader);

    // The cgss::Sprite renders with bound RenderStates via
    // bindRenderStates() — but litergss3's Sprite uses immediate-mode
    // Ops::draw_texture_pro and doesn't currently consult render states.
    // Storing the shader at the Ruby level preserves API compatibility;
    // wiring it through to the draw path lands when the Sprite renderer
    // adopts render-state-aware drawing (Phase 5 / shader work).
    return shader;
}

void Init_ShaderSprite()
{
    rb_cShaderSprite = rb_define_class_under(rb_mLiteRGSS, "ShaderedSprite", rb_cSprite);
    // Same allocator as Sprite — ShaderedSprite shares SpriteData layout.
    rb_define_alloc_func(rb_cShaderSprite, rb::Alloc<SpriteData>);
    rb_define_method(rb_cShaderSprite, "shader", _rbf rb_ShaderSprite_getShader, 0);
    rb_define_method(rb_cShaderSprite, "shader=", _rbf rb_ShaderSprite_setShader, 1);
    rb_define_method(rb_cShaderSprite, "blendmode", _rbf rb_ShaderSprite_getShader, 0);
    rb_define_method(rb_cShaderSprite, "blendmode=", _rbf rb_ShaderSprite_setShader, 1);
}
