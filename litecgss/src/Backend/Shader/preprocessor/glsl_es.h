#pragma once

// GL ES adaptation of shaders written for the desktop fixed pipeline (GLSL
// 1.10 - 1.30 with the compatibility built-ins). Used by ShaderPreprocessor
// only when LiteCGSS runs on GL ES; see ShaderPreprocessor.h, steps 7 to 9.

#include "../ShaderPreprocessor.h"

#include <string>
#include <string_view>
#include <vector>

namespace cgss::glsl {

// True for a desktop "#version" directive ("#version 130", "#version 120
// compatibility"...), false for GLSL ES ones ("#version 100", "#version 300 es").
bool isDesktopVersionDirective(std::string_view directive);

// Vertex inputs the rewritten legacy vertex shaders are wired to.
enum class VertexInterface {
	// SFML ES (see sf::RenderTarget): position / texCoord / color attributes,
	// sf_modelview / sf_projection / sf_texture uniforms, NPOT factor_npot.
	SfmlEs,
	// raylib: vertexPosition / vertexTexCoord / vertexColor attributes and the
	// mvp uniform; texture coordinates are never padded.
	Raylib,
};

// Rewrites a vertex shader using the fixed-pipeline built-ins on `iface`'s
// attributes / uniforms, writing the varyings the fragment rewrite reads. Its
// gl_TexCoord are over the image, without the NPOT padding, like on desktop.
// Returns the source unchanged when it doesn't use them, or already handles
// GL ES itself (mentions GL_ES, declares attributes).
std::string rewriteLegacyVertex(const std::string& source, VertexInterface iface = VertexInterface::SfmlEs);

// Resolves `#ifdef GL_ES` / `#ifndef GL_ES` / `#if [!]defined(GL_ES)` blocks
// (with their #else) for a compiler that does (`glEs`) or doesn't define
// GL_ES: the dead branch and the directives go, the live branch stays. Other
// conditionals are kept as they are. Lets a shader written for SFML ES with a
// desktop fallback be rewritten as a desktop one.
std::string resolveGlEsConditionals(const std::string& source, bool glEs);

// Makes a vertex shader handling GL ES itself, which writes
// v_texture_coordinates, write v_factor_npot too: the fragment rewrite reads
// it. Returns the source unchanged when it already does.
std::string writeNpotVarying(const std::string& source);

// Makes the source acceptable to a GLSL ES compiler: removes uniform
// initializers (appending their values to `defaults`) and turns integer
// literals next to a float operand into float literals.
std::string enforceEsStrictness(const std::string& source,
                                std::vector<ShaderPreprocessor::UniformDefault>& defaults);

} // namespace cgss::glsl
