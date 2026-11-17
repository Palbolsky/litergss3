#pragma once

// GLSL 330 core conversion, the last step of ShaderPreprocessor's
// Target::RaylibGlsl330: the earlier steps have rewritten the fixed-pipeline
// built-ins onto plain varyings / attributes; this pass makes the remaining
// GLSL 1.x syntax core-profile GLSL 330:
//   - `#version` -> `#version 330` (`#extension` lines kept right after it),
//   - `attribute` -> `in`; `varying` -> `out` (vertex) / `in` (fragment),
//   - texture2D / texture2DProj / texture2DLod / textureCube(Lod) ->
//     texture / textureProj / textureLod,
//   - gl_FragColor -> `out vec4 cgss_FragColor`,
//   - the `texture` sampler -> texture0 (`texture` names the sampling function
//     in GLSL 330; texture0 is the sampler raylib binds the drawn texture to),
//   - identifiers that are reserved words in GLSL 330 get a `_` suffix.
// Comments and preprocessor lines pass through verbatim.

#include <string>
#include <string_view>

namespace cgss::glsl {

// `es`: GLSL ES 3.00 (`#version 300 es`) instead: the same syntax for what the
// preprocessor emits, and fragment shaders get `precision highp float;` first
// (GLSL ES has no default float precision there).
std::string toGlsl330(const std::string& source, bool vertexStage, bool es = false);

// True when `source` is already a toGlsl330 output (keeps processing idempotent).
bool isGlsl330Processed(std::string_view source);

// Name of a uniform of the original source in the converted one.
std::string glsl330UniformName(const std::string& name);

} // namespace cgss::glsl
