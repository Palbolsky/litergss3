#ifndef CGSS_SHADER_TYPE_H
#define CGSS_SHADER_TYPE_H

// Backend-agnostic shader stage enum; each backend maps it to its native
// shader stage in its Ops.
namespace cgss {

	enum class ShaderType : int {
		Vertex   = 0,
		Geometry = 1,
		Fragment = 2
	};

	// Tag type used to bind the current target texture to a shader sampler.
	struct CurrentTextureType {};
	inline constexpr CurrentTextureType CurrentTexture {};

}

#endif
