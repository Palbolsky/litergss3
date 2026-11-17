#include "glsl_330.h"

#include "glsl_lexer.h"

#include <algorithm>
#include <initializer_list>

namespace cgss::glsl {

namespace {

constexpr std::string_view kMarker = "// [LiteCGSS::ShaderPreprocessor glsl330]\n";

bool startsWithDirective(std::string_view preproc, std::string_view directive) {
	std::size_t i = 1; // past '#'
	while (i < preproc.size() && (preproc[i] == ' ' || preproc[i] == '\t')) ++i;
	return preproc.compare(i, directive.size(), directive) == 0;
}

bool isOneOf(std::string_view s, std::initializer_list<std::string_view> list) {
	return std::find(list.begin(), list.end(), s) != list.end();
}

// Words GLSL 1.x shaders may use as identifiers but GLSL 330 reserves.
bool isReservedIn330(std::string_view s) {
	return isOneOf(s, { "sample", "layout", "patch", "subroutine", "smooth", "flat",
	                    "noperspective", "centroid", "precise", "resource", "filter" });
}

std::string_view textureFunction(std::string_view s) {
	if (s == "texture2D" || s == "textureCube") return "texture";
	if (s == "texture2DProj") return "textureProj";
	if (s == "texture2DLod" || s == "textureCubeLod") return "textureLod";
	return {};
}

} // namespace

bool isGlsl330Processed(std::string_view source) {
	return source.substr(0, 256).find(kMarker) != std::string_view::npos;
}

std::string glsl330UniformName(const std::string& name) {
	if (name == "texture") return "texture0";
	if (isReservedIn330(name)) return name + "_";
	return name;
}

// A `#define` line, its replacement list converted like code (the sampler
// `texture` -> texture0, texture2D & co, reserved words), its macro name kept:
// `#define TEX texture` must still name the sampler once it's renamed.
static std::string convertDefine(std::string_view directive) {
	const std::size_t keyword = directive.find("define");
	if (keyword == std::string_view::npos) return std::string(directive);
	std::string out(directive.substr(0, keyword + 6));
	const std::string rest(directive.substr(keyword + 6));
	const auto tokens = lex(rest);
	bool nameSeen = false;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		const std::string_view s = text(tokens[i], rest);
		if (tokens[i].kind != TokenKind::Ident) {
			out.append(s);
		} else if (!nameSeen) {
			nameSeen = true;
			out.append(s);
		} else if (const auto fn = textureFunction(s); !fn.empty()) {
			out.append(fn);
		} else if (s == "texture") {
			const std::size_t next = skipTrivia(tokens, i + 1);
			const bool call = next < tokens.size() && isPunctChar(tokens[next], rest, '(');
			out.append(call ? "texture" : "texture0");
		} else if (isReservedIn330(s)) {
			out.append(s);
			out.push_back('_');
		} else {
			out.append(s);
		}
	}
	return out;
}

std::string toGlsl330(const std::string& source, bool vertexStage, bool es) {
	if (isGlsl330Processed(source)) return source;
	const auto tokens = lex(source);

	std::string extensions;
	std::string body;
	body.reserve(source.size() + 64);
	bool usesFragColor = false;

	for (std::size_t i = 0; i < tokens.size(); ++i) {
		const Token& t = tokens[i];
		const std::string_view s = text(t, source);

		if (t.kind == TokenKind::Preproc && (startsWithDirective(s, "version") || startsWithDirective(s, "extension"))) {
			if (startsWithDirective(s, "extension")) {
				extensions.append(s);
				extensions.push_back('\n');
			}
			if (i + 1 < tokens.size() && tokens[i + 1].kind == TokenKind::Newline) ++i;
			continue;
		}
		if (t.kind == TokenKind::Preproc && startsWithDirective(s, "define")) {
			body.append(convertDefine(s));
			continue;
		}
		if (t.kind != TokenKind::Ident) {
			body.append(s);
			continue;
		}

		if (s == "attribute") {
			body.append("in");
		} else if (s == "varying") {
			body.append(vertexStage ? "out" : "in");
		} else if (const auto fn = textureFunction(s); !fn.empty()) {
			body.append(fn);
		} else if (s == "gl_FragColor" && !vertexStage) {
			body.append("cgss_FragColor");
			usesFragColor = true;
		} else if (s == "texture") {
			// The GLSL 1.30+ function `texture(` stays; the sampler is renamed.
			const std::size_t next = skipTrivia(tokens, i + 1);
			const bool call = next < tokens.size() && isPunctChar(tokens[next], source, '(');
			body.append(call ? "texture" : "texture0");
		} else if (isReservedIn330(s)) {
			body.append(s);
			body.push_back('_');
		} else {
			body.append(s);
		}
	}

	std::string out = es ? "#version 300 es\n" : "#version 330\n";
	// GLSL ES fragment shaders have no default float precision, and the
	// declarations below come before the precision block: highp from the start.
	if (es && !vertexStage) out.append("precision highp float;\n");
	out.append(kMarker);
	out.append(extensions);
	if (usesFragColor) out.append("out vec4 cgss_FragColor;\n");
	out.append(body);
	return out;
}

} // namespace cgss::glsl
