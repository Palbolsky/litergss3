#ifndef CGSS_GLSL_H
#define CGSS_GLSL_H

// Backend-agnostic GLSL uniform value types. Layouts are chosen to be
// structurally compatible with sf::Glsl::* counterparts so that conversion
// at the backend boundary (see Graphics/Internal/SfShaderConv.h) is trivial.
// No SFML dependency.
namespace cgss {
namespace Glsl {

	struct Vec2 { float x = 0.f; float y = 0.f; };
	struct Vec3 { float x = 0.f; float y = 0.f; float z = 0.f; };
	struct Vec4 { float x = 0.f; float y = 0.f; float z = 0.f; float w = 0.f; };

	struct Ivec2 { int x = 0; int y = 0; };
	struct Ivec3 { int x = 0; int y = 0; int z = 0; };
	struct Ivec4 { int x = 0; int y = 0; int z = 0; int w = 0; };

	struct Bvec2 { bool x = false; bool y = false; };
	struct Bvec3 { bool x = false; bool y = false; bool z = false; };
	struct Bvec4 { bool x = false; bool y = false; bool z = false; bool w = false; };

	struct Mat3 { float array[9] = {}; };
	struct Mat4 { float array[16] = {}; };

}
}

#endif
