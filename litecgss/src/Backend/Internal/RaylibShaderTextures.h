#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_SHADER_TEXTURES_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_SHADER_TEXTURES_H

// Extra textures of a shader (sampler uniforms other than the drawn one).
//
// raylib's SetShaderValueTexture keeps the texture in rlgl's batch slots,
// which are only 4 (RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS, a build constant)
// and are emptied by every batch flush: a texture set once, when the shader
// is created, is gone by the time it draws, and a 5th one never binds (its
// sampler then reads the drawn texture: PSDK's battle transitions showed the
// map tiled). The bindings are kept here instead, per shader, and bound when
// the shader draws on texture units rlgl doesn't use (5 and up).

namespace cgss::backend::raylib_detail {

	void setShaderTexture(unsigned int shaderId, int samplerLocation, unsigned int textureId);
	// Before drawing with the shader (enabled): binds its textures.
	void bindShaderTextures(unsigned int shaderId);
	// The program goes away (unloaded or reloaded).
	void forgetShaderTextures(unsigned int shaderId);

}

#endif
