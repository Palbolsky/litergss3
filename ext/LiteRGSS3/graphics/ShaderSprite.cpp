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

    // Wire the shader through to the cgss::Sprite so its draw path picks
    // up the bound RenderStates (BeginShaderMode/EndShaderMode under
    // raylib, equivalent under SFML). The @shader ivar above pins the
    // Ruby wrapper so its underlying cgss::RenderStates outlives the
    // binding for GC purposes.
    auto *s = rb::GetPtr<SpriteData>(self);
    if (s != nullptr && s->has_sprite) {
        if (NIL_P(shader)) {
            s->sprite->bindRenderStates(nullptr);
        } else {
            s->sprite->bindRenderStates(rb::GetPtr<RenderStatesElement>(shader));
        }
    }
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
