#ifndef CGSS_SHADER_TYPE_H
#define CGSS_SHADER_TYPE_H

// Backend-agnostic shader stage enum. Numeric values match sf::Shader::Type
// so that static_cast at the backend boundary is safe.
namespace cgss {

	enum class ShaderType : int {
		Vertex   = 0,
		Geometry = 1,
		Fragment = 2
	};

	// Tag type used to bind the current target texture to a shader sampler —
	// mirrors sf::Shader::CurrentTextureType.
	struct CurrentTextureType {};
	inline constexpr CurrentTextureType CurrentTexture {};

}

#endif
