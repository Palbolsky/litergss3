#include "ShaderPreprocessor.h"
#include "Common/Profiling.h"

#include "preprocessor/glsl_330.h"
#include "preprocessor/glsl_es.h"
#include "preprocessor/glsl_lexer.h"
#include "preprocessor/glsl_rewrite.h"
#include "preprocessor/glsl_symbols.h"

#include <cstring>
#include <string>
#include <string_view>


namespace {

// --- Static GLSL blocks -----------------------------------------------------

// On GL ES 2 the fragment shader has no default float precision and must
// declare one explicitly. mediump is a 16-bit float (~10-bit mantissa) which
// is exact for integer UVs only up to ~2048 px — above that, sampling snaps
// to coarser texel grids and large sprite sheets (e.g. long flipbook strips)
// pixelate near their right/bottom edges. highp is the fix; it's optional in
// ES 2 fragment shaders but flagged by the shader compiler's own
// GL_FRAGMENT_PRECISION_HIGH macro, so the runtime branch is free of any
// C++-side capability check. mediump fallback preserves prior behavior on
// the (rare) devices without highp support.
constexpr const char* kPrecisionBlock =
	"#ifdef GL_ES\n"
	"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"precision highp sampler2D;\n"
	"#else\n"
	"precision mediump float;\n"
	"#endif\n"
	"#endif\n";

// Varyings/fallbacks the preprocessor injects at the top of fragment shaders.
// Note: v_factor_npot is exposed mainly as an escape hatch for legacy shaders
// that still reference it directly; new shaders should work in logical UV
// space and ignore it entirely.
constexpr const char* kFragmentVaryingHeader =
	"#ifdef GL_ES\n"
	"varying vec2 v_texture_coordinates;\n"
	"varying vec2 v_inv_texture_coordinates;\n"
	"varying vec4 sf_color;\n"
	"varying vec2 v_factor_npot;\n"
	"#else\n"
	"const vec2 v_factor_npot = vec2(1.0, 1.0);\n"
	"#endif\n";

// raylib's vertex rewrite writes the same varyings on desktop GL too.
constexpr const char* kRaylibFragmentVaryingHeader =
	"varying vec2 v_texture_coordinates;\n"
	"varying vec2 v_inv_texture_coordinates;\n"
	"varying vec4 sf_color;\n"
	"varying vec2 v_factor_npot;\n";

// Sentinel comment prepended to every processed fragment shader. If we see it
// at the top of a source on entry, the shader has already been through
// process() and we return it unchanged — that's what makes process() safely
// idempotent in the face of (a) the user feeding back already-rewritten source
// and (b) tests that call process() twice as a smoke check. The marker is
// stable across format tweaks because it's just one line we control.
constexpr const char* kProcessedMarker = "// [LiteCGSS::ShaderPreprocessor processed]\n";

constexpr std::string_view kVersion = "version";

bool alreadyProcessed(const std::string& source) {
	return source.compare(0, std::strlen(kProcessedMarker), kProcessedMarker) == 0;
}

struct SplittedShaderSource {
	// Contains leading comments/space up until the #version directive, or "" if no directive
	std::string prologue;
	// Contains the body after the #version directive, or the whole shader if no directive
	std::string body;
	bool hasVersion = false;
	// Index of the directive's '#' in prologue
	std::size_t directiveStart = 0;
};

constexpr bool isHorizontalSpace(char c) {
	return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}
constexpr bool isLineTerminator(char c) {
	return c == '\n' || c == '\r';
}

std::size_t skipInitialSpaceAndComments(std::string_view source) {
	std::size_t i = 0;
	while (i < source.size()) {
		const char c = source[i];
		if (isHorizontalSpace(c) || isLineTerminator(c)) {
			++i;
			continue;
		}
		if (c == '/' && i + 1 < source.size()) {
			if (source[i + 1] == '/') {
				i += 2;
				while (i < source.size() && !isLineTerminator(source[i])) {
					++i;
				}
				continue;
			}
			if (source[i + 1] == '*') {
				const auto end = source.find("*/", i + 2);
				if (end == std::string::npos) {
					return std::string::npos;
				}
				i = end + 2;
				continue;
			}
		}
		return i;
	}
	return i;
}


// Index just past the terminator ending the line containing i
std::size_t endOfLine(std::string_view source, std::size_t i) {
	while (i < source.size() && !isLineTerminator(source[i])) {
		++i;
	}
	if (i == source.size()) {
		return i;
	}
	const char first = source[i++];
	if (i < source.size() && isLineTerminator(source[i]) && source[i] != first) {
		++i; // \r\n, \n\r
	}
	return i;
}

// Split a shader into its prologue (leading comments/spaces + version directive) and its body
// In the case no version directive is provided, the comments ends up in the body
SplittedShaderSource splitShader(std::string_view source) {
	SplittedShaderSource out;

	// First look for the end of initial whitespaces/comments.
	const std::size_t start = skipInitialSpaceAndComments(source);
	if (start == std::string::npos || start >= source.size() || source[start] != '#') {
		out.body = source;
		return out;
	}

	// Look for the #version directive
	std::size_t i = start + 1;
	while (i < source.size() && isHorizontalSpace(source[i])) ++i; // "#  version" is legal
	if (source.compare(i, kVersion.size(), kVersion) != 0) {
		out.body = source;
		return out;
	}
	i += kVersion.size();

	// Must be followed by a separator; "#versionfoo" is an identifier, not a directive.
	if (i < source.size() && !isHorizontalSpace(source[i]) && !isLineTerminator(source[i])) {
		out.body = source;
		return out;
	}

	const std::size_t end = endOfLine(source, start);
	out.prologue       = source.substr(0, end);
	out.body           = source.substr(end);
	out.hasVersion     = true;
	out.directiveStart = start;
	return out;
}

void addVersionIfMissing(SplittedShaderSource& source, std::string_view versionDirective) {
	if (!source.hasVersion && !versionDirective.empty()) {
		source.prologue = "#version " + std::string(versionDirective) + "\n";
		source.hasVersion = true;
	}
}

// GL ES only accepts GLSL ES versions: a shader written for desktop GL
// ("#version 130", as older PSDK versions prepend to every shader) gets the
// GLSL ES 1.00 one, the rest of the preprocessing making it fit.
void replaceDesktopVersion(SplittedShaderSource& source) {
	if (!source.hasVersion) return;
	if (cgss::glsl::isDesktopVersionDirective(std::string_view(source.prologue).substr(source.directiveStart))) {
		source.prologue = source.prologue.substr(0, source.directiveStart) + "#version 100\n";
	}
}

using Target = cgss::ShaderPreprocessor::Target;

// Targets on which the fixed-pipeline built-ins aren't fed and get rewritten.
bool rewritesBuiltins(Target target) {
	return target != Target::DesktopFixedPipeline;
}

cgss::glsl::VertexInterface vertexInterface(Target target) {
	return cgss::ShaderPreprocessor::isRaylibTarget(target) ? cgss::glsl::VertexInterface::Raylib : cgss::glsl::VertexInterface::SfmlEs;
}

// Desktop GL's default vertex shader, as SFML writes it: on GL ES / raylib the
// legacy vertex rewrite turns it into one writing the fragment stage's varyings.
constexpr const char* kFixedPipelineVertexShader =
	"void main()\n"
	"{\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
	"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
	"    gl_FrontColor = gl_Color;\n"
	"}\n";

// Desktop GL's fixed-pipeline fragment stage: texture times vertex color.
constexpr const char* kFixedPipelineFragmentShader =
	"uniform sampler2D texture;\n"
	"void main()\n"
	"{\n"
	"    gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;\n"
	"}\n";

// Fragment-stage entry point: lex → discover → rewrite → wrap with static
// headers/marker. The vertex/geometry stages don't currently get any
// rewriting; they pass through untouched aside from the optional #version.
std::string processFragmentModern(const std::string& source, Target target) {
	const auto tokens = cgss::glsl::lex(source);
	const auto sym    = cgss::glsl::discoverSymbols(tokens, source);
	std::string rewritten = cgss::glsl::rewriteAndEmit(tokens, source, sym, rewritesBuiltins(target));

	rewritten = std::string(cgss::ShaderPreprocessor::isRaylibTarget(target) ? kRaylibFragmentVaryingHeader : kFragmentVaryingHeader) + rewritten;
	rewritten = std::string(kPrecisionBlock) + rewritten;
	rewritten = std::string(kProcessedMarker) + rewritten;
	return rewritten;
}

} // namespace

namespace cgss {

std::string ShaderPreprocessor::process(const std::string& source, Stage stage) {
	CGSS_PROFILE_ZONE("cgss::ShaderPreprocessor::process");
	return process(source, stage, Options{});
}

std::string ShaderPreprocessor::process(const std::string& source, Stage stage, const Options& opts) {
	CGSS_PROFILE_ZONE("cgss::ShaderPreprocessor::process");
	return processDetailed(source, stage, opts).source;
}

ShaderPreprocessor::Result ShaderPreprocessor::processDetailed(const std::string& source, Stage stage) {
	return processDetailed(source, stage, Options{});
}

ShaderPreprocessor::Result ShaderPreprocessor::processDetailed(const std::string& source, Stage stage, const Options& opts) {
	Result result;
	if (isRaylibTarget(opts.target) && cgss::glsl::isGlsl330Processed(source)) {
		result.source = source;
		return result;
	}
	auto splittedShader = splitShader(source);
	if (isRaylibTarget(opts.target)) {
		// Keep the desktop branches of a GL ES-aware shader, which the rewrite
		// below then handles: its GL ES branches are written for SFML ES'
		// attributes, not raylib's (and raylib's desktop GL never defines GL_ES).
		std::string desktop = cgss::glsl::resolveGlEsConditionals(splittedShader.body, false);
		if (stage == Stage::Vertex && cgss::glsl::declaresSfmlEsInterface(desktop)) {
			// Unless that desktop branch is written for SFML ES' attributes too
			// (no built-in to rewrite): then the GL ES branch, the complete one
			// (PSDK-android's loader background, which writes its varyings
			// there only), is mapped onto raylib's attributes.
			splittedShader.body = cgss::glsl::adaptSfmlEsVertex(
				cgss::glsl::resolveGlEsConditionals(splittedShader.body, true));
		} else {
			splittedShader.body = std::move(desktop);
		}
	}

	if (opts.target == Target::SfmlEs100) {
		replaceDesktopVersion(splittedShader);
	}
	addVersionIfMissing(splittedShader, opts.versionDirective);

	if (stage == Stage::Fragment && !alreadyProcessed(splittedShader.body)) {
		splittedShader.body = processFragmentModern(splittedShader.body, opts.target);
	}
	if (rewritesBuiltins(opts.target) && stage == Stage::Vertex) {
		splittedShader.body = cgss::glsl::rewriteLegacyVertex(splittedShader.body, vertexInterface(opts.target));
		splittedShader.body = cgss::glsl::writeNpotVarying(splittedShader.body);
	}
	if (opts.target == Target::SfmlEs100 || opts.target == Target::RaylibGlslEs300) {
		splittedShader.body = cgss::glsl::enforceEsStrictness(splittedShader.body, result.uniformDefaults,
			opts.target == Target::SfmlEs100);
	}

	result.source = splittedShader.prologue + splittedShader.body;
	if (isRaylibTarget(opts.target)) {
		result.source = cgss::glsl::toGlsl330(result.source, stage == Stage::Vertex,
			opts.target == Target::RaylibGlslEs300);
	}
	return result;
}

const std::string& ShaderPreprocessor::defaultVertexShader() {
	return defaultVertexShader(Target::SfmlEs100);
}

const std::string& ShaderPreprocessor::defaultVertexShader(Target target) {
	static const std::string shaders[] = {
		process(kFixedPipelineVertexShader, Stage::Vertex, Options{ Target::DesktopFixedPipeline }),
		process(kFixedPipelineVertexShader, Stage::Vertex, Options{ Target::SfmlEs100 }),
		process(kFixedPipelineVertexShader, Stage::Vertex, Options{ Target::RaylibGlsl330 }),
		process(kFixedPipelineVertexShader, Stage::Vertex, Options{ Target::RaylibGlslEs300 }),
	};
	return shaders[static_cast<int>(target)];
}

const std::string& ShaderPreprocessor::defaultFragmentShader(Target target) {
	static const std::string shaders[] = {
		process(kFixedPipelineFragmentShader, Stage::Fragment, Options{ Target::DesktopFixedPipeline }),
		process(kFixedPipelineFragmentShader, Stage::Fragment, Options{ Target::SfmlEs100 }),
		process(kFixedPipelineFragmentShader, Stage::Fragment, Options{ Target::RaylibGlsl330 }),
		process(kFixedPipelineFragmentShader, Stage::Fragment, Options{ Target::RaylibGlslEs300 }),
	};
	return shaders[static_cast<int>(target)];
}

std::string ShaderPreprocessor::uniformName(const std::string& name, Target target) {
	return isRaylibTarget(target) ? cgss::glsl::glsl330UniformName(name) : name;
}

} // namespace cgss
