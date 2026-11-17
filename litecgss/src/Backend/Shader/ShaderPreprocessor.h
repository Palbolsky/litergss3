#pragma once

#include <string>
#include <vector>

namespace cgss {

// Preprocesses GLSL source so that user-authored shaders can work in a single
// "logical UV space [0,1] per sampler" convention regardless of whether the
// underlying platform supports non-power-of-two textures (desktop GL with
// ARB_texture_non_power_of_two, vs GL ES 2 without OES_texture_npot).
//
// Concretely, this module performs the same job that the PSDK-side
// Shader.rb auto-adapter used to do, but inside the engine so that:
//   - every consumer of LiteCGSS benefits without re-implementing it,
//   - the conversion is uniform across samplers (not just the primary one
//     bound via the sprite bitmap),
//   - user shader source can stay free of v_factor_npot ceremony.
//
// The transformations applied (in order):
//
//   1. Inject GL_ES varyings / desktop fallbacks for v_texture_coordinates,
//      v_inv_texture_coordinates, sf_color and v_factor_npot.
//   2. Strip author-side redeclarations of v_factor_npot (so the injected
//      header never collides with hand-written defensive declarations).
//   3. Prepend the standard "#ifdef GL_ES precision mediump float; #endif"
//      precision block.
//   4. Rewrite deprecated fixed-pipeline references:
//        - gl_TexCoord[0] -> (v_texture_coordinates / v_factor_npot)
//          (always normalised to logical [0,1], on both desktop and GL_ES)
//        - gl_TexCoord[1] -> (v_inv_texture_coordinates / v_factor_npot)
//        - gl_Color       -> sf_color
//   5. Wrap every texture2D(sampler, uv) call into
//      texture2D(sampler, (uv) * <sampler>_factor_npot), using a paren-
//      balanced parser so calls whose UV expression contains commas/parens
//      are still rewritten correctly. Each discovered sampler gets a
//      companion "uniform vec2 <sampler>_factor_npot;" injected near the
//      top of the shader. The engine pushes the matching value whenever a
//      texture is bound to that sampler (see RenderStatesData::setShaderUniform).
//   6. Prepend "#version <SHADER_VERSION>" if the source does not declare one
//      itself. The exact version string is platform-dependent and supplied
//      by the caller (or defaults to empty for SFML's auto-detection).
//
// Steps 4 (gl_TexCoord / gl_Color rewrite) and 7 to 9 depend on the Target
// (Options::target), chosen by the graphics backend: desktop GL with the
// fixed-pipeline built-ins skips them; the others need them.
//
// On GL ES (Target::SfmlEs100), shaders written for the desktop
// fixed pipeline (e.g. games made with an older PSDK) are made GLSL ES 1.00:
//
//   7. A desktop "#version" (110, 120, 130...) becomes "#version 100".
//   8. Vertex shaders using the fixed-pipeline built-ins (gl_Vertex,
//      gl_MultiTexCoord0, gl_TextureMatrix[0], gl_ModelViewProjectionMatrix,
//      gl_Color, gl_FrontColor, gl_TexCoord) are rewritten on SFML ES's
//      attributes and uniforms, and write the varyings of step 1. Their
//      gl_TexCoord leave the NPOT padding out, like on desktop. A vertex
//      shader handling GL_ES itself (or declaring attributes) is left alone,
//      but for writing v_factor_npot, which step 1 needs for linking.
//   9. What GLSL ES 1.00 lacks is rewritten:
//        - uniform initializers are removed and reported (processDetailed);
//        - an integer literal next to a float operand, or passed to a float
//          built-in, becomes a float literal ("1 - uv.y", "sqrt(2)");
//        - `a % b` -> `int(mod(float(a), float(b)))`;
//        - round() gets float / vec2-4 overloads;
//        - a file-scope const array becomes one constant per element and an
//          accessor function (GLSL ES 1.00 only guarantees constant indices
//          in fragment shaders).
//
// The preprocessor is intentionally idempotent: running it on already-
// processed source is a no-op (modulo stripping a still-present author
// redeclaration), which is useful for tests and for shaders that include
// each other.
class ShaderPreprocessor {
public:
	enum class Stage {
		Vertex,
		Fragment,
		Geometry
	};

	// What the processed shader is compiled for. The backend picks it
	// (Backend::Ops::shader_preprocessor_target).
	enum class Target {
		// Desktop GL keeping the fixed-pipeline built-ins (stock SFML): legacy
		// sources keep gl_TexCoord / gl_Color; only the NPOT handling applies.
		DesktopFixedPipeline,
		// GL ES 2 through SFML ES (the SFML-opengl-es2 fork): GLSL ES 1.00 on
		// SFML ES's attributes and uniforms (steps 7 to 9).
		SfmlEs100,
		// raylib on desktop GL 3.3+: the SFML ES rewrite (steps 4 and 8) on
		// raylib's attributes (vertexPosition / vertexTexCoord / vertexColor,
		// mvp), then GLSL 330 core (#version 330, in / out, texture(),
		// cgss_FragColor, the `texture` sampler renamed texture0 — the one
		// raylib binds the drawn texture to; see uniformName()).
		RaylibGlsl330,
		// raylib on GL ES 3 (Android): RaylibGlsl330's rewrite, as GLSL ES
		// 3.00 (#version 300 es, highp in fragment shaders through the
		// precision block), with what GLSL ES forbids rewritten (uniform
		// initializers, integer literals next to float operands — step 9's
		// first two; 3.00 has the rest).
		RaylibGlslEs300,
	};

	// Targets on raylib's attributes and uniforms (RaylibGlsl330, RaylibGlslEs300).
	static constexpr bool isRaylibTarget(Target target) {
		return target == Target::RaylibGlsl330 || target == Target::RaylibGlslEs300;
	}

	struct Options {
		Target target = Target::SfmlEs100;

		// If non-empty, prepended verbatim as "#version <versionDirective>\n"
		// when the source lacks its own #version directive.
		std::string versionDirective {};

		// Reserved for future use: color / tone / alpha process injection.
		// Currently handled on the PSDK side; will migrate here once the
		// convention has stabilised.
		bool addColorProcess = false;
		bool addToneProcess = false;
		bool addAlphaProcess = false;
	};

	// Value given to a uniform by its declaration (`uniform float opacity = 1.0;`).
	// GLSL ES forbids such initializers: the preprocessor removes them and
	// reports the values, for the caller to set once the shader is linked.
	struct UniformDefault {
		std::string type;          // "float", "vec2".."vec4", "int" or "bool"
		std::string name;
		std::vector<float> values; // one per component
	};

	struct Result {
		std::string source;
		std::vector<UniformDefault> uniformDefaults;
	};

	// Returns the processed shader source, ready to be handed to
	// sf::Shader::loadFromMemory. The original source is not modified.
	// Two overloads: the no-options form is the common case; pass an explicit
	// Options struct only when a non-default version directive is needed.
	static std::string process(const std::string& source, Stage stage);
	static std::string process(const std::string& source, Stage stage, const Options& opts);
	// Same, with the uniform defaults removed from the source (GL ES only).
	static Result processDetailed(const std::string& source, Stage stage);
	static Result processDetailed(const std::string& source, Stage stage, const Options& opts);

	// Vertex shader paired with a fragment-only shader: GL ES cannot link a
	// program without one. It writes the varyings the fragment rewrite reads.
	static const std::string& defaultVertexShader();
	static const std::string& defaultVertexShader(Target target);
	// Fragment shader paired with a vertex-only shader, for targets whose
	// default fragment stage doesn't read the vertex rewrite's varyings.
	static const std::string& defaultFragmentShader(Target target);

	// Name a uniform of the original source has in the processed one.
	static std::string uniformName(const std::string& name, Target target);
};

} // namespace cgss
