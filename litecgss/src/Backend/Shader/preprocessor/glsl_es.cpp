#include "glsl_es.h"

#include "glsl_lexer.h"
#include "glsl_symbols.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string_view>

namespace cgss::glsl {

namespace {

// Same contract as the rewrite pass: every token is passed, replaced or
// skipped, with optional text inserted before it.
struct Edit {
	enum class Kind { Pass, Replace, Skip };
	Kind        kind = Kind::Pass;
	std::string replacement;
	std::string insertBefore;
};

std::string emit(const std::vector<Token>& tokens, std::string_view source, const std::vector<Edit>& edits) {
	std::string out;
	out.reserve(source.size() + 512);
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		out.append(edits[i].insertBefore);
		switch (edits[i].kind) {
			case Edit::Kind::Pass:    out.append(text(tokens[i], source)); break;
			case Edit::Kind::Replace: out.append(edits[i].replacement);    break;
			case Edit::Kind::Skip:    break;
		}
	}
	return out;
}

bool contains(std::initializer_list<std::string_view> list, std::string_view s) {
	return std::find(list.begin(), list.end(), s) != list.end();
}

std::size_t prevNonTrivia(const std::vector<Token>& tokens, std::size_t i) {
	while (i > 0) {
		--i;
		if (!is_trivia(tokens[i].kind)) return i;
	}
	return SymbolTable::npos;
}

// --- Legacy vertex shaders --------------------------------------------------

constexpr const char* kProcessedMarker = "// [LiteCGSS::ShaderPreprocessor processed]\n";

// SFML ES's vertex interface (see sf::RenderTarget), the varyings the
// fragment rewrite reads, and globals standing for the fixed-pipeline
// built-ins. The built-ins are globals filled by main() rather than direct
// references to the attributes: a legacy shader may declare a local named
// like an attribute (PSDK's map_viewport.vert has a `vec2 position`).
// sf_TextureMatrix leaves the NPOT padding out of the coordinates, as a legacy
// shader computing with them (PSDK's battle_shadow.vert) expects 0 to 1 over
// the whole image.
constexpr const char* kLegacyVertexHeader =
	"attribute vec2 position;\n"
	"attribute vec2 texCoord;\n"
	"attribute vec4 color;\n"
	"uniform mat4 sf_modelview;\n"
	"uniform mat4 sf_projection;\n"
	"uniform mat4 sf_texture;\n"
	"uniform vec2 factor_npot;\n"
	"varying vec2 v_texture_coordinates;\n"
	"varying vec2 v_inv_texture_coordinates;\n"
	"varying vec4 sf_color;\n"
	"varying vec2 v_factor_npot;\n"
	"vec4 sf_Vertex;\n"
	"vec4 sf_MultiTexCoord0;\n"
	"vec4 sf_Color;\n"
	"mat4 sf_ModelViewProjectionMatrix;\n"
	"mat4 sf_TextureMatrix;\n"
	"vec4 sf_TexCoord[2];\n";

// The shader's own main() is renamed; this one sets the built-ins up with
// the values the fixed pipeline would give them, runs it, then hands the
// texture coordinates to the fragment stage, which reads them padded.
constexpr const char* kLegacyVertexMain =
	"\nvoid main()\n"
	"{\n"
	"    sf_Vertex = vec4(position, 0.0, 1.0);\n"
	"    sf_MultiTexCoord0 = vec4(texCoord, 0.0, 1.0);\n"
	"    sf_Color = color;\n"
	"    sf_ModelViewProjectionMatrix = sf_projection * sf_modelview;\n"
	"    sf_color = sf_Color;\n"
	"    sf_TextureMatrix = mat4(1.0 / factor_npot.x, 0.0, 0.0, 0.0,\n"
	"                            0.0, 1.0 / factor_npot.y, 0.0, 0.0,\n"
	"                            0.0, 0.0, 1.0, 0.0,\n"
	"                            0.0, 0.0, 0.0, 1.0) * sf_texture;\n"
	"    sf_TexCoord[0] = sf_TextureMatrix * sf_MultiTexCoord0;\n"
	"    sf_TexCoord[1] = sf_TexCoord[0];\n"
	"    sf_legacy_main();\n"
	"    v_texture_coordinates = sf_TexCoord[0].xy * factor_npot;\n"
	"    v_inv_texture_coordinates = sf_TexCoord[1].xy * factor_npot;\n"
	"    v_factor_npot = factor_npot;\n"
	"}\n";

// raylib's counterpart. Its draws only provide the combined mvp matrix, so
// the separate model-view / projection built-ins become identity / mvp; and
// raylib never pads textures, so the texture matrix is the identity.
constexpr const char* kRaylibLegacyVertexHeader =
	"attribute vec3 vertexPosition;\n"
	"attribute vec2 vertexTexCoord;\n"
	"attribute vec4 vertexColor;\n"
	"uniform mat4 mvp;\n"
	"varying vec2 v_texture_coordinates;\n"
	"varying vec2 v_inv_texture_coordinates;\n"
	"varying vec4 sf_color;\n"
	"varying vec2 v_factor_npot;\n"
	"vec4 sf_Vertex;\n"
	"vec4 sf_MultiTexCoord0;\n"
	"vec4 sf_Color;\n"
	"mat4 sf_ModelViewProjectionMatrix;\n"
	"mat4 sf_modelview;\n"
	"mat4 sf_projection;\n"
	"mat4 sf_TextureMatrix;\n"
	"vec4 sf_TexCoord[2];\n";

constexpr const char* kRaylibLegacyVertexMain =
	"\nvoid main()\n"
	"{\n"
	"    sf_Vertex = vec4(vertexPosition, 1.0);\n"
	"    sf_MultiTexCoord0 = vec4(vertexTexCoord, 0.0, 1.0);\n"
	"    sf_Color = vertexColor;\n"
	"    sf_ModelViewProjectionMatrix = mvp;\n"
	"    sf_modelview = mat4(1.0);\n"
	"    sf_projection = mvp;\n"
	"    sf_color = sf_Color;\n"
	"    sf_TextureMatrix = mat4(1.0);\n"
	"    sf_TexCoord[0] = sf_TextureMatrix * sf_MultiTexCoord0;\n"
	"    sf_TexCoord[1] = sf_TexCoord[0];\n"
	"    sf_legacy_main();\n"
	"    v_texture_coordinates = sf_TexCoord[0].xy;\n"
	"    v_inv_texture_coordinates = sf_TexCoord[1].xy;\n"
	"    v_factor_npot = vec2(1.0, 1.0);\n"
	"}\n";

// Names the raylib header declares: a shader resolved from a GL ES-aware one
// may declare them itself (its desktop branch writes v_factor_npot).
bool isRaylibHeaderVarying(std::string_view s) {
	return contains({ "v_texture_coordinates", "v_inv_texture_coordinates", "sf_color", "v_factor_npot" }, s);
}

// Drops top-level `varying <type> <name>;` declarations of header names.
void stripHeaderVaryingRedeclarations(const std::vector<Token>& tokens, std::string_view source, std::vector<Edit>& edits) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident || text(tokens[i], source) != "varying") continue;
		const std::size_t type = skipTrivia(tokens, i + 1);
		const std::size_t name = type < tokens.size() ? skipTrivia(tokens, type + 1) : tokens.size();
		const std::size_t semi = name < tokens.size() ? skipTrivia(tokens, name + 1) : tokens.size();
		if (semi >= tokens.size() || !isPunctChar(tokens[semi], source, ';')) continue;
		if (!isRaylibHeaderVarying(text(tokens[name], source))) continue;
		std::size_t end = semi + 1;
		if (end < tokens.size() && tokens[end].kind == TokenKind::Newline) ++end;
		for (std::size_t k = i; k < end; ++k) edits[k].kind = Edit::Kind::Skip;
		i = end - 1;
	}
}

bool isLegacyVertexBuiltin(std::string_view s) {
	return contains({ "gl_Vertex", "gl_MultiTexCoord0", "gl_TextureMatrix", "gl_ModelViewProjectionMatrix",
	                  "gl_ModelViewMatrix", "gl_ProjectionMatrix", "gl_Color", "gl_FrontColor", "gl_TexCoord" }, s);
}

bool isLegacyVertexShader(const std::vector<Token>& tokens, std::string_view source) {
	// A shader handling GL ES itself mentions GL_ES in code (comments aside).
	for (const auto& t : tokens) {
		if ((t.kind == TokenKind::Ident || t.kind == TokenKind::Preproc)
		    && text(t, source).find("GL_ES") != std::string_view::npos) return false;
	}
	bool usesBuiltins = false;
	for (const auto& t : tokens) {
		if (t.kind != TokenKind::Ident) continue;
		const std::string_view s = text(t, source);
		if (s == "attribute") return false;
		usesBuiltins = usesBuiltins || isLegacyVertexBuiltin(s);
	}
	return usesBuiltins;
}

// The fragment rewrite reads v_factor_npot whatever the vertex shader: this
// main() writes it for a shader handling GL ES without it, as a varying the
// vertex stage doesn't write doesn't link.
constexpr const char* kNpotVaryingMain =
	"void main()\n"
	"{\n"
	"    sf_author_main();\n"
	"    v_factor_npot = factor_npot;\n"
	"}\n";

bool hasIdent(const std::vector<Token>& tokens, std::string_view source, std::string_view name) {
	return std::any_of(tokens.begin(), tokens.end(), [&](const Token& t) {
		return t.kind == TokenKind::Ident && text(t, source) == name;
	});
}

// --- Float operand detection ------------------------------------------------

bool isFloatType(std::string_view s) {
	return contains({ "float", "vec2", "vec3", "vec4", "mat2", "mat3", "mat4" }, s);
}

// GLSL ES 1.00 built-in functions all take and return floats (genType).
bool isFloatBuiltinFunction(std::string_view s) {
	return contains({ "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "pow", "exp", "log",
	                  "exp2", "log2", "sqrt", "inversesqrt", "abs", "sign", "floor", "ceil", "fract", "mod",
	                  "min", "max", "clamp", "mix", "step", "smoothstep", "length", "distance", "dot", "cross",
	                  "normalize", "faceforward", "reflect", "refract", "matrixCompMult",
	                  "texture2D", "texture2DProj", "texture2DLod", "textureCube", "textureCubeLod",
	                  "round" /* polyfilled */ }, s);
}

bool isFloatBuiltinVariable(std::string_view s) {
	return contains({ "gl_Position", "gl_PointSize", "gl_FragColor", "gl_FragData", "gl_FragCoord", "gl_PointCoord" }, s);
}

bool isIntegerLiteral(std::string_view s) {
	return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}

bool isFloatLiteral(const Token& t, std::string_view source) {
	if (t.kind != TokenKind::Number) return false;
	const std::string_view s = text(t, source);
	if (s.size() > 1 && (s[1] == 'x' || s[1] == 'X')) return false;
	return s.find_first_of(".eE") != std::string_view::npos;
}

// Identifiers declared with a float type (variables, parameters, uniforms...)
// and functions returning one. Scopes aren't tracked: a name declared float
// anywhere counts as float everywhere, which only matters if the shader
// reuses the name for an int.
struct FloatSymbols {
	std::vector<std::string> variables;
	std::vector<std::string> functions;

	bool isVariable(std::string_view s) const {
		return isFloatBuiltinVariable(s) || std::find(variables.begin(), variables.end(), s) != variables.end();
	}
	bool isFunction(std::string_view s) const {
		return isFloatType(s) || isFloatBuiltinFunction(s)
		    || std::find(functions.begin(), functions.end(), s) != functions.end();
	}
};

FloatSymbols discoverFloatSymbols(const std::vector<Token>& tokens, std::string_view source) {
	FloatSymbols sym;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident || !isFloatType(text(tokens[i], source))) continue;
		std::size_t j = skipTrivia(tokens, i + 1);
		if (j >= tokens.size() || tokens[j].kind != TokenKind::Ident) continue; // constructor call
		const std::string name(text(tokens[j], source));
		const std::size_t k = skipTrivia(tokens, j + 1);
		if (k < tokens.size() && isPunctChar(tokens[k], source, '(')) {
			sym.functions.push_back(name);
			continue;
		}
		sym.variables.push_back(name);
		// Further declarators: `vec2 a = ..., b;` (not `, vec2 b` parameters,
		// which the loop meets on their own type token).
		int depth = 0;
		for (j = k; j < tokens.size(); ++j) {
			if (isPunctChar(tokens[j], source, '(') || isPunctChar(tokens[j], source, '[')) ++depth;
			else if (isPunctChar(tokens[j], source, ')') || isPunctChar(tokens[j], source, ']')) { if (--depth < 0) break; }
			else if (depth == 0 && (isPunctChar(tokens[j], source, ';') || isPunctChar(tokens[j], source, '{'))) break;
			else if (depth == 0 && isPunctChar(tokens[j], source, ',')) {
				// A declarator name is followed by punctuation (`, b;`, `, b = 1.0`);
				// an identifier followed by another one is a parameter's type.
				const std::size_t n = skipTrivia(tokens, j + 1);
				if (n >= tokens.size() || tokens[n].kind != TokenKind::Ident) break;
				const std::size_t after = skipTrivia(tokens, n + 1);
				if (after < tokens.size() && tokens[after].kind == TokenKind::Ident) break;
				sym.variables.emplace_back(text(tokens[n], source));
			}
		}
	}
	return sym;
}

// Index of the bracket matching tokens[i], which is one of ( ) [ ]: searched
// forward from an opening bracket, backward from a closing one. npos if none.
std::size_t matchingBracket(const std::vector<Token>& tokens, std::string_view source, std::size_t i) {
	const char self    = text(tokens[i], source)[0];
	const bool forward = self == '(' || self == '[';
	const char other   = self == '(' ? ')' : self == ')' ? '(' : self == '[' ? ']' : '[';
	int depth = 0;
	for (std::size_t k = i; k < tokens.size();) {
		if (isPunctChar(tokens[k], source, self)) ++depth;
		else if (isPunctChar(tokens[k], source, other) && --depth == 0) return k;
		if (forward) ++k;
		else if (k-- == 0) break;
	}
	return SymbolTable::npos;
}

// A parenthesized group is taken as float when it holds any float literal or
// float symbol: mixing them with ints would be an error in GLSL ES anyway.
bool groupIsFloat(const std::vector<Token>& tokens, std::string_view source, const FloatSymbols& sym, std::size_t open, std::size_t close) {
	for (std::size_t k = open + 1; k < close; ++k) {
		if (isFloatLiteral(tokens[k], source)) return true;
		if (tokens[k].kind != TokenKind::Ident) continue;
		const std::string_view s = text(tokens[k], source);
		if (sym.isVariable(s)) return true;
		const std::size_t n = skipTrivia(tokens, k + 1);
		if (sym.isFunction(s) && n < tokens.size() && isPunctChar(tokens[n], source, '(')) return true;
	}
	return false;
}

// Operand starting at tokens[i] (right of an operator).
bool operandStartsFloat(const std::vector<Token>& tokens, std::string_view source, const FloatSymbols& sym, std::size_t i) {
	i = skipTrivia(tokens, i);
	if (i < tokens.size() && (isPunctChar(tokens[i], source, '-') || isPunctChar(tokens[i], source, '+'))) {
		i = skipTrivia(tokens, i + 1);
	}
	if (i >= tokens.size()) return false;
	if (isFloatLiteral(tokens[i], source)) return true;
	if (tokens[i].kind == TokenKind::Ident) {
		const std::string_view s = text(tokens[i], source);
		const std::size_t n = skipTrivia(tokens, i + 1);
		if (n < tokens.size() && isPunctChar(tokens[n], source, '(')) return sym.isFunction(s);
		return sym.isVariable(s);
	}
	if (isPunctChar(tokens[i], source, '(')) {
		const std::size_t close = matchingBracket(tokens, source, i);
		return close != SymbolTable::npos && groupIsFloat(tokens, source, sym, i, close);
	}
	return false;
}

// Operand ending at tokens[i] (left of an operator): walks back over
// `.swizzle`, `[index]` and call parentheses to the expression's base.
bool operandEndsFloat(const std::vector<Token>& tokens, std::string_view source, const FloatSymbols& sym, std::size_t i) {
	while (i != SymbolTable::npos) {
		if (isFloatLiteral(tokens[i], source)) return true;
		if (tokens[i].kind == TokenKind::Ident) {
			const std::size_t p = prevNonTrivia(tokens, i);
			if (p != SymbolTable::npos && isPunctChar(tokens[p], source, '.')) {
				i = prevNonTrivia(tokens, p); // swizzle / field: look at what it applies to
				continue;
			}
			return sym.isVariable(text(tokens[i], source));
		}
		if (isPunctChar(tokens[i], source, ']')) {
			const std::size_t open = matchingBracket(tokens, source, i);
			if (open == SymbolTable::npos) return false;
			i = prevNonTrivia(tokens, open);
			continue;
		}
		if (isPunctChar(tokens[i], source, ')')) {
			const std::size_t open = matchingBracket(tokens, source, i);
			if (open == SymbolTable::npos) return false;
			const std::size_t callee = prevNonTrivia(tokens, open);
			if (callee != SymbolTable::npos && tokens[callee].kind == TokenKind::Ident) {
				return sym.isFunction(text(tokens[callee], source));
			}
			return groupIsFloat(tokens, source, sym, open, i);
		}
		return false;
	}
	return false;
}

bool isBinaryOperator(const Token& t, std::string_view source) {
	if (t.kind != TokenKind::Punct) return false;
	return contains({ "+", "-", "*", "/", "<", ">", "<=", ">=", "==", "!=" }, text(t, source));
}

bool isAssignment(const Token& t, std::string_view source) {
	return t.kind == TokenKind::Punct && contains({ "=", "+=", "-=", "*=", "/=" }, text(t, source));
}

// A token after which an operator is binary (it ends an operand).
bool endsOperand(const Token& t, std::string_view source) {
	return t.kind == TokenKind::Ident || t.kind == TokenKind::Number
	    || isPunctChar(t, source, ')') || isPunctChar(t, source, ']');
}

// True when tokens[i] is a whole argument (a unary sign allowed) of a call to
// a float built-in function.
bool isFloatBuiltinArgument(const std::vector<Token>& tokens, std::string_view source, std::size_t i) {
	const std::size_t next = skipTrivia(tokens, i + 1);
	if (next >= tokens.size() || !(isPunctChar(tokens[next], source, ',') || isPunctChar(tokens[next], source, ')'))) return false;
	std::size_t p = prevNonTrivia(tokens, i);
	if (p != SymbolTable::npos && (isPunctChar(tokens[p], source, '-') || isPunctChar(tokens[p], source, '+'))) {
		p = prevNonTrivia(tokens, p);
	}
	if (p == SymbolTable::npos || !(isPunctChar(tokens[p], source, ',') || isPunctChar(tokens[p], source, '('))) return false;
	// Enclosing call's '(' (skipping the previous arguments)
	int depth = 0;
	for (std::size_t k = p; k != SymbolTable::npos; k = prevNonTrivia(tokens, k)) {
		if (isPunctChar(tokens[k], source, ')') || isPunctChar(tokens[k], source, ']')) ++depth;
		else if (isPunctChar(tokens[k], source, '[')) --depth;
		else if (isPunctChar(tokens[k], source, '(') && depth-- == 0) {
			const std::size_t callee = prevNonTrivia(tokens, k);
			return callee != SymbolTable::npos && tokens[callee].kind == TokenKind::Ident
			    && isFloatBuiltinFunction(text(tokens[callee], source));
		}
		if (depth < 0) return false;
	}
	return false;
}

void promoteIntegerLiterals(const std::vector<Token>& tokens, std::string_view source, std::vector<Edit>& edits) {
	const FloatSymbols sym = discoverFloatSymbols(tokens, source);
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Number || !isIntegerLiteral(text(tokens[i], source))) continue;
		if (edits[i].kind == Edit::Kind::Skip) continue; // removed initializer

		bool promote = false;
		// `<operand> op N` / `x = N`, skipping a unary sign: `x * -1`
		std::size_t op = prevNonTrivia(tokens, i);
		if (op != SymbolTable::npos && (isPunctChar(tokens[op], source, '-') || isPunctChar(tokens[op], source, '+'))) {
			const std::size_t before = prevNonTrivia(tokens, op);
			if (before == SymbolTable::npos || !endsOperand(tokens[before], source)) op = before;
		}
		if (op != SymbolTable::npos && (isBinaryOperator(tokens[op], source) || isAssignment(tokens[op], source))) {
			const std::size_t lhs = prevNonTrivia(tokens, op);
			promote = lhs != SymbolTable::npos && endsOperand(tokens[lhs], source) && operandEndsFloat(tokens, source, sym, lhs);
		}
		// `sqrt(2)`, `mod(x, 1)`: GLSL ES 1.00 built-ins only take floats
		// (constructors do convert, so `vec2(1, 0)` stays)
		if (!promote) promote = isFloatBuiltinArgument(tokens, source, i);
		// `N op <operand>`
		if (!promote) {
			const std::size_t next = skipTrivia(tokens, i + 1);
			if (next < tokens.size() && isBinaryOperator(tokens[next], source)) {
				promote = operandStartsFloat(tokens, source, sym, next + 1);
			}
		}
		if (promote) {
			edits[i].kind        = Edit::Kind::Replace;
			edits[i].replacement = std::string(text(tokens[i], source)) + ".0";
		}
	}
}

// --- Uniform initializers ---------------------------------------------------

std::size_t componentCount(std::string_view type) {
	if (type == "vec2") return 2;
	if (type == "vec3") return 3;
	if (type == "vec4") return 4;
	return 1;
}

// Parses a constant initializer: a number, true / false, or a constructor of
// numbers (`vec2(0.1, 0.1)`, `vec4(0.0)`). Leaves `values` empty otherwise.
std::vector<float> parseInitializer(const std::vector<Token>& tokens, std::string_view source, std::size_t from, std::size_t to,
                                    std::string_view type) {
	std::vector<float> values;
	std::size_t i = skipTrivia(tokens, from);
	if (i < to && tokens[i].kind == TokenKind::Ident) {
		const std::string_view s = text(tokens[i], source);
		if (s == "true" || s == "false") {
			return skipTrivia(tokens, i + 1) == to ? std::vector<float>{ s == "true" ? 1.f : 0.f } : values;
		}
		i = skipTrivia(tokens, i + 1);
		if (i >= to || !isPunctChar(tokens[i], source, '(')) return values;
		++i;
	}
	for (bool expectValue = true; i < to; ) {
		i = skipTrivia(tokens, i);
		if (i >= to) break;
		if (expectValue) {
			float sign = 1.f;
			if (isPunctChar(tokens[i], source, '-')) { sign = -1.f; i = skipTrivia(tokens, i + 1); }
			if (i >= to || tokens[i].kind != TokenKind::Number) return {};
			values.push_back(sign * std::strtof(std::string(text(tokens[i], source)).c_str(), nullptr));
			expectValue = false;
		} else if (isPunctChar(tokens[i], source, ',')) {
			expectValue = true;
		} else if (!isPunctChar(tokens[i], source, ')')) {
			return {};
		}
		++i;
	}
	if (values.size() == 1) values.resize(componentCount(type), values[0]); // vec4(0.0)
	if (values.size() != componentCount(type)) return {};
	return values;
}

// `uniform [precision] <type> name = init[, name2 = init2];`: removes each
// ` = init` and records its value.
void stripUniformInitializers(const std::vector<Token>& tokens, std::string_view source, std::vector<Edit>& edits,
                              std::vector<ShaderPreprocessor::UniformDefault>& defaults) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident || text(tokens[i], source) != "uniform") continue;
		std::size_t t = skipTrivia(tokens, i + 1);
		while (t < tokens.size() && contains({ "lowp", "mediump", "highp" }, text(tokens[t], source))) {
			t = skipTrivia(tokens, t + 1);
		}
		if (t >= tokens.size() || tokens[t].kind != TokenKind::Ident) continue;
		const std::string type(text(tokens[t], source));

		int depth = 0;
		std::size_t name = SymbolTable::npos;
		std::size_t assign = SymbolTable::npos;
		for (std::size_t k = t + 1; k < tokens.size(); ++k) {
			if (isPunctChar(tokens[k], source, '(') || isPunctChar(tokens[k], source, '[')) { ++depth; continue; }
			if (isPunctChar(tokens[k], source, ')') || isPunctChar(tokens[k], source, ']')) { --depth; continue; }
			if (depth != 0) continue;
			const bool end = isPunctChar(tokens[k], source, ';') || isPunctChar(tokens[k], source, ',');
			if (end && assign != SymbolTable::npos) {
				// From the spaces before '=' up to the terminator
				std::size_t from = assign;
				while (from > 0 && tokens[from - 1].kind == TokenKind::Whitespace) --from;
				for (std::size_t s = from; s < k; ++s) edits[s].kind = Edit::Kind::Skip;
				if (name != SymbolTable::npos) {
					auto values = parseInitializer(tokens, source, assign + 1, k, type);
					if (!values.empty()) {
						defaults.push_back({ type, std::string(text(tokens[name], source)), std::move(values) });
					}
				}
				assign = SymbolTable::npos;
			}
			if (isPunctChar(tokens[k], source, ';')) { i = k; break; }
			if (end) { name = SymbolTable::npos; continue; }
			if (isPunctChar(tokens[k], source, '=')) { assign = k; continue; }
			if (assign == SymbolTable::npos && tokens[k].kind == TokenKind::Ident) name = k;
		}
	}
}

// --- round() ----------------------------------------------------------------

// GLSL 1.30's round() doesn't exist in GLSL ES 1.00 (halfway cases may go
// either way in GLSL anyway)
constexpr const char* kRoundPolyfill =
	"// --- round() for GLSL ES 1.00 (auto-generated) ---\n"
	"float round(float x) { return floor(x + 0.5); }\n"
	"vec2 round(vec2 x) { return floor(x + 0.5); }\n"
	"vec3 round(vec3 x) { return floor(x + 0.5); }\n"
	"vec4 round(vec4 x) { return floor(x + 0.5); }\n"
	"// -------------------------------------------------\n";

void addRoundPolyfill(const std::vector<Token>& tokens, std::string_view source, const SymbolTable& sym, std::vector<Edit>& edits) {
	if (sym.firstDefinitionTok == SymbolTable::npos) return;
	for (const auto& fn : sym.functions) {
		if (fn.name == "round") return; // the shader has its own
	}
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident || text(tokens[i], source) != "round") continue;
		const std::size_t n = skipTrivia(tokens, i + 1);
		if (n < tokens.size() && isPunctChar(tokens[n], source, '(')) {
			edits[sym.firstDefinitionTok].insertBefore += kRoundPolyfill;
			return;
		}
	}
}

// --- Integer modulus ----------------------------------------------------------

// First token of the primary expression ending at tokens[i]:
// `a.b[1].c`, `f(x)`, `(expr)`, with a unary sign / `!`.
std::size_t primaryStart(const std::vector<Token>& tokens, std::string_view source, std::size_t i) {
	std::size_t start = i;
	while (i != SymbolTable::npos) {
		if (isPunctChar(tokens[i], source, ']') || isPunctChar(tokens[i], source, ')')) {
			const std::size_t open = matchingBracket(tokens, source, i);
			if (open == SymbolTable::npos) return start;
			start = open;
			const std::size_t before = prevNonTrivia(tokens, open);
			if (before == SymbolTable::npos || tokens[before].kind != TokenKind::Ident) break;
			i = before; // indexed / called expression
			continue;
		}
		start = i;
		const std::size_t before = prevNonTrivia(tokens, i);
		if (tokens[i].kind == TokenKind::Ident && before != SymbolTable::npos && isPunctChar(tokens[before], source, '.')) {
			i = prevNonTrivia(tokens, before); // field / swizzle
			continue;
		}
		break;
	}
	const std::size_t sign = prevNonTrivia(tokens, start);
	if (sign != SymbolTable::npos && (isPunctChar(tokens[sign], source, '-') || isPunctChar(tokens[sign], source, '+')
	                                  || isPunctChar(tokens[sign], source, '!'))) {
		const std::size_t before = prevNonTrivia(tokens, sign);
		if (before == SymbolTable::npos || !endsOperand(tokens[before], source)) start = sign;
	}
	return start;
}

// Last token of the primary expression starting after tokens[i].
std::size_t primaryEnd(const std::vector<Token>& tokens, std::string_view source, std::size_t i) {
	i = skipTrivia(tokens, i);
	if (i < tokens.size() && (isPunctChar(tokens[i], source, '-') || isPunctChar(tokens[i], source, '+'))) {
		i = skipTrivia(tokens, i + 1);
	}
	if (i >= tokens.size()) return SymbolTable::npos;
	std::size_t end = i;
	if (isPunctChar(tokens[i], source, '(')) {
		end = matchingBracket(tokens, source, i);
	} else if (tokens[i].kind == TokenKind::Ident) {
		const std::size_t n = skipTrivia(tokens, i + 1);
		if (n < tokens.size() && isPunctChar(tokens[n], source, '(')) end = matchingBracket(tokens, source, n);
	}
	// Postfix: `[index]`, `.field`
	while (end != SymbolTable::npos) {
		const std::size_t n = skipTrivia(tokens, end + 1);
		if (n < tokens.size() && isPunctChar(tokens[n], source, '[')) {
			end = matchingBracket(tokens, source, n);
		} else if (n < tokens.size() && isPunctChar(tokens[n], source, '.')) {
			const std::size_t field = skipTrivia(tokens, n + 1);
			if (field >= tokens.size() || tokens[field].kind != TokenKind::Ident) break;
			end = field;
		} else {
			break;
		}
	}
	return end;
}

// `a % b` (GLSL ES 3.00 only) -> `int(mod(float(a), float(b)))`. `*`, `/`
// and `%` share the precedence of `%`: the left operand spans such a chain.
void rewriteIntegerModulus(const std::vector<Token>& tokens, std::string_view source, std::vector<Edit>& edits) {
	for (std::size_t op = 0; op < tokens.size(); ++op) {
		if (tokens[op].kind != TokenKind::Punct || text(tokens[op], source) != "%") continue;
		if (edits[op].kind == Edit::Kind::Skip) continue;
		const std::size_t lhsEnd = prevNonTrivia(tokens, op);
		const std::size_t rhsEnd = primaryEnd(tokens, source, op + 1);
		if (lhsEnd == SymbolTable::npos || rhsEnd == SymbolTable::npos || rhsEnd + 1 >= tokens.size()) continue;
		std::size_t lhsStart = primaryStart(tokens, source, lhsEnd);
		for (std::size_t chained = prevNonTrivia(tokens, lhsStart); chained != SymbolTable::npos;
		     chained = prevNonTrivia(tokens, lhsStart)) {
			const std::string_view t = text(tokens[chained], source);
			if (tokens[chained].kind != TokenKind::Punct || (t != "*" && t != "/" && t != "%")) break;
			const std::size_t before = prevNonTrivia(tokens, chained);
			if (before == SymbolTable::npos) break;
			lhsStart = primaryStart(tokens, source, before);
		}
		edits[lhsStart].insertBefore += "int(mod(float(";
		edits[op].kind        = Edit::Kind::Replace;
		edits[op].replacement = "), float(";
		edits[rhsEnd + 1].insertBefore = ")))" + edits[rhsEnd + 1].insertBefore;
		// `a % b` -> `float(a), float(b)`, not `float(a ), float( b)`
		for (std::size_t k = lhsEnd + 1; k < op; ++k) {
			if (tokens[k].kind == TokenKind::Whitespace) edits[k].kind = Edit::Kind::Skip;
		}
		for (std::size_t k = op + 1; k < tokens.size() && tokens[k].kind == TokenKind::Whitespace; ++k) {
			edits[k].kind = Edit::Kind::Skip;
		}
	}
}

// --- Constant arrays ----------------------------------------------------------

// Text of tokens [from, to) with their edits applied (comments dropped,
// newlines as spaces), trimmed.
std::string emitExpression(const std::vector<Token>& tokens, std::string_view source, const std::vector<Edit>& edits,
                           std::size_t from, std::size_t to) {
	std::string out;
	for (std::size_t k = from; k < to; ++k) {
		if (tokens[k].kind == TokenKind::LineComment || tokens[k].kind == TokenKind::BlockComment) continue;
		out += edits[k].insertBefore;
		if (edits[k].kind == Edit::Kind::Skip) continue;
		if (tokens[k].kind == TokenKind::Newline) out += ' ';
		else out += edits[k].kind == Edit::Kind::Replace ? std::string_view(edits[k].replacement) : text(tokens[k], source);
	}
	const auto first = out.find_first_not_of(" \t");
	if (first == std::string::npos) return std::string();
	return out.substr(first, out.find_last_not_of(" \t") - first + 1);
}

// `const <type> NAME[N] = <type>[N](e0, e1, ...);` at file scope (array
// initializers are GLSL ES 3.00 only) -> one constant per element and
// `<type> NAME_at(int)` returning them, every `NAME[index]` -> `NAME_at(index)`.
// GLSL ES 1.00 only guarantees constant-index-expressions when indexing in
// a fragment shader; this works with any index. Only const arrays: they
// can't be written to, so the accessor covers every use.
void rewriteConstArrays(const std::vector<Token>& tokens, std::string_view source, std::vector<Edit>& edits) {
	int depth = 0;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (isPunctChar(tokens[i], source, '{')) ++depth;
		else if (isPunctChar(tokens[i], source, '}')) --depth;
		if (depth != 0 || tokens[i].kind != TokenKind::Ident || text(tokens[i], source) != "const") continue;

		// const <type> NAME [ N? ] = <type> [ N? ] ( args ) ;
		const std::size_t type = skipTrivia(tokens, i + 1);
		if (type >= tokens.size() || tokens[type].kind != TokenKind::Ident) continue;
		const std::size_t name = skipTrivia(tokens, type + 1);
		if (name >= tokens.size() || tokens[name].kind != TokenKind::Ident) continue;
		const std::size_t open = skipTrivia(tokens, name + 1);
		if (open >= tokens.size() || !isPunctChar(tokens[open], source, '[')) continue;
		const std::size_t close = matchingBracket(tokens, source, open);
		if (close == SymbolTable::npos) continue;
		const std::size_t assign = skipTrivia(tokens, close + 1);
		if (assign >= tokens.size() || !isPunctChar(tokens[assign], source, '=')) continue;
		const std::size_t ctor = skipTrivia(tokens, assign + 1);
		if (ctor >= tokens.size() || text(tokens[ctor], source) != text(tokens[type], source)) continue;
		const std::size_t ctorOpen = skipTrivia(tokens, ctor + 1);
		if (ctorOpen >= tokens.size() || !isPunctChar(tokens[ctorOpen], source, '[')) continue;
		const std::size_t ctorClose = matchingBracket(tokens, source, ctorOpen);
		const std::size_t argsOpen = ctorClose == SymbolTable::npos ? ctorClose : skipTrivia(tokens, ctorClose + 1);
		if (argsOpen >= tokens.size() || !isPunctChar(tokens[argsOpen], source, '(')) continue;
		const std::size_t argsClose = matchingBracket(tokens, source, argsOpen);
		if (argsClose == SymbolTable::npos) continue;
		const std::size_t semicolon = skipTrivia(tokens, argsClose + 1);
		if (semicolon >= tokens.size() || !isPunctChar(tokens[semicolon], source, ';')) continue;

		const std::string typeName(text(tokens[type], source));
		const std::string arrayName(text(tokens[name], source));

		// Elements, split on the call's top-level commas
		std::vector<std::string> elements;
		std::size_t argStart = argsOpen + 1;
		int argDepth = 0;
		for (std::size_t k = argsOpen + 1; k <= argsClose; ++k) {
			if (k < argsClose) {
				if (isPunctChar(tokens[k], source, '(') || isPunctChar(tokens[k], source, '[')) { ++argDepth; continue; }
				if (isPunctChar(tokens[k], source, ')') || isPunctChar(tokens[k], source, ']')) { --argDepth; continue; }
				if (argDepth != 0 || !isPunctChar(tokens[k], source, ',')) continue;
			}
			std::string element = emitExpression(tokens, source, edits, argStart, k);
			if (typeName == "float" && isIntegerLiteral(element)) element += ".0"; // `float[](1, 2)`
			elements.push_back(std::move(element));
			argStart = k + 1;
		}
		if (elements.empty()) continue;

		std::string replacement;
		for (std::size_t e = 0; e < elements.size(); ++e) {
			replacement += "const " + typeName + " " + arrayName + "_" + std::to_string(e) + " = " + elements[e] + ";\n";
		}
		replacement += typeName + " " + arrayName + "_at(int sf_index) {\n";
		for (std::size_t e = 0; e + 1 < elements.size(); ++e) {
			replacement += "    if (sf_index == " + std::to_string(e) + ") return " + arrayName + "_" + std::to_string(e) + ";\n";
		}
		replacement += "    return " + arrayName + "_" + std::to_string(elements.size() - 1) + ";\n}";

		edits[i].kind        = Edit::Kind::Replace;
		edits[i].replacement = std::move(replacement);
		for (std::size_t k = i + 1; k <= semicolon; ++k) edits[k] = Edit{ Edit::Kind::Skip, {}, {} };

		// Uses: NAME[index] -> NAME_at(index)
		for (std::size_t k = semicolon + 1; k < tokens.size(); ++k) {
			if (tokens[k].kind != TokenKind::Ident || text(tokens[k], source) != arrayName) continue;
			const std::size_t index = skipTrivia(tokens, k + 1);
			if (index >= tokens.size() || !isPunctChar(tokens[index], source, '[')) continue;
			const std::size_t indexEnd = matchingBracket(tokens, source, index);
			if (indexEnd == SymbolTable::npos) continue;
			edits[k].kind = Edit::Kind::Replace;
			edits[k].replacement = arrayName + "_at";
			edits[index].kind = Edit::Kind::Replace;
			edits[index].replacement = "(";
			edits[indexEnd].kind = Edit::Kind::Replace;
			edits[indexEnd].replacement = ")";
		}
		i = semicolon;
	}
}

} // namespace

bool isDesktopVersionDirective(std::string_view directive) {
	// "#version <number> [profile]": a directive is one Preproc token for the
	// lexer, so it is read by hand.
	const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
	std::size_t i = directive.find("version");
	if (i == std::string_view::npos) return false;
	i += 7;
	while (i < directive.size() && isSpace(directive[i])) ++i;
	const std::size_t numberStart = i;
	while (i < directive.size() && directive[i] >= '0' && directive[i] <= '9') ++i;
	const std::string_view number = directive.substr(numberStart, i - numberStart);
	while (i < directive.size() && isSpace(directive[i])) ++i;
	const std::string_view profile = directive.substr(i, directive.find_first_of(" \t\r\n", i) - i);
	return !number.empty() && number != "100" && profile != "es";
}

namespace {

bool isSfmlEsAttribute(std::string_view s) { return contains({ "position", "texCoord", "color" }, s); }
bool isSfmlEsUniform(std::string_view s) {
	return contains({ "sf_modelview", "sf_projection", "sf_texture", "factor_npot" }, s);
}
bool isPrecisionQualifier(std::string_view s) { return contains({ "lowp", "mediump", "highp" }, s); }

// A top-level `attribute|uniform [precision] <type> <name>;` declaration of
// one of `isName`'s names at token i: its type and name tokens, and the end.
struct Declaration { std::size_t type, name, end; };
std::optional<Declaration> declarationAt(const std::vector<Token>& tokens, std::string_view source, std::size_t i,
                                         std::string_view keyword, bool (*isName)(std::string_view)) {
	if (tokens[i].kind != TokenKind::Ident || text(tokens[i], source) != keyword) return std::nullopt;
	std::size_t type = skipTrivia(tokens, i + 1);
	if (type < tokens.size() && isPrecisionQualifier(text(tokens[type], source))) type = skipTrivia(tokens, type + 1);
	const std::size_t name = type < tokens.size() ? skipTrivia(tokens, type + 1) : tokens.size();
	const std::size_t semi = name < tokens.size() ? skipTrivia(tokens, name + 1) : tokens.size();
	if (semi >= tokens.size() || !isPunctChar(tokens[semi], source, ';')) return std::nullopt;
	if (tokens[name].kind != TokenKind::Ident || !isName(text(tokens[name], source))) return std::nullopt;
	std::size_t end = semi + 1;
	if (end < tokens.size() && tokens[end].kind == TokenKind::Newline) ++end;
	return Declaration{ type, name, end };
}

// The global an attribute becomes, from raylib's
std::string attributeValue(std::string_view name, std::string_view type) {
	if (name == "position") {
		if (type == "vec2") return "vertexPosition.xy";
		if (type == "vec4") return "vec4(vertexPosition, 1.0)";
		return "vertexPosition";
	}
	if (name == "texCoord") return type == "vec4" ? "vec4(vertexTexCoord, 0.0, 1.0)" : "vertexTexCoord";
	return type == "vec3" ? "vertexColor.rgb" : "vertexColor";
}

}  // namespace

bool declaresSfmlEsInterface(const std::string& source) {
	const auto tokens = lex(source);
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (declarationAt(tokens, source, i, "attribute", isSfmlEsAttribute)) return true;
		if (auto d = declarationAt(tokens, source, i, "uniform", isSfmlEsUniform);
		    d && text(tokens[d->name], source) != "factor_npot") return true;
	}
	return false;
}

std::string adaptSfmlEsVertex(const std::string& source) {
	const auto tokens = lex(source);
	const SymbolTable sym = discoverSymbols(tokens, source);
	std::vector<Edit> edits(tokens.size());

	std::string globals;
	std::string setup;
	bool hasFactorNpot = false;
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		auto attribute = declarationAt(tokens, source, i, "attribute", isSfmlEsAttribute);
		auto uniform   = attribute ? std::nullopt : declarationAt(tokens, source, i, "uniform", isSfmlEsUniform);
		const auto& d  = attribute ? attribute : uniform;
		if (!d) continue;
		const std::string type(text(tokens[d->type], source));
		const std::string name(text(tokens[d->name], source));
		globals += type + " " + name + ";\n";
		if (attribute)                   setup += "    " + name + " = " + attributeValue(name, type) + ";\n";
		else if (name == "sf_projection") setup += "    sf_projection = mvp;\n";
		else if (name == "factor_npot")  { setup += "    factor_npot = vec2(1.0, 1.0);\n"; hasFactorNpot = true; }
		else                             setup += "    " + name + " = mat4(1.0);\n";
		for (std::size_t k = i; k < d->end; ++k) edits[k].kind = Edit::Kind::Skip;
		i = d->end - 1;
	}
	// The fragment rewrite reads v_factor_npot: writeNpotVarying reads this one
	if (!hasFactorNpot) {
		globals += "vec2 factor_npot;\n";
		setup   += "    factor_npot = vec2(1.0, 1.0);\n";
	}
	for (const auto& fn : sym.functions) {
		if (fn.name == "main") {
			edits[fn.nameTok].kind        = Edit::Kind::Replace;
			edits[fn.nameTok].replacement = "sf_es_main";
		}
	}
	// The varyings the fragment rewrite reads, from raylib's attributes, before
	// the shader's own main(), which may write them otherwise: a shader that
	// doesn't write one still links (the fragment stage reads all of them)
	std::string defaults;
	static constexpr struct { const char* type; const char* name; const char* value; } kVaryings[] = {
		{ "vec4", "sf_color", "vertexColor" },
		{ "vec2", "v_texture_coordinates", "vertexTexCoord" },
		{ "vec2", "v_inv_texture_coordinates", "vertexTexCoord" },
		{ "vec2", "v_factor_npot", "vec2(1.0, 1.0)" },
	};
	for (const auto& v : kVaryings) {
		if (!hasIdent(tokens, source, v.name)) globals += std::string("varying ") + v.type + " " + v.name + ";\n";
		defaults += std::string("    ") + v.name + " = " + v.value + ";\n";
	}
	setup += defaults;
	return "attribute vec3 vertexPosition;\n"
	       "attribute vec2 vertexTexCoord;\n"
	       "attribute vec4 vertexColor;\n"
	       "uniform mat4 mvp;\n" + globals
	     + emit(tokens, source, edits)
	     + "\nvoid main()\n{\n" + setup + "    sf_es_main();\n}\n";
}

std::string rewriteLegacyVertex(const std::string& source, VertexInterface iface) {
	const auto tokens = lex(source);
	if (!isLegacyVertexShader(tokens, source)) return source;
	const SymbolTable sym = discoverSymbols(tokens, source);

	std::vector<Edit> edits(tokens.size());
	for (const auto& fn : sym.functions) {
		if (fn.name == "main") {
			edits[fn.nameTok].kind        = Edit::Kind::Replace;
			edits[fn.nameTok].replacement = "sf_legacy_main";
		}
	}
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident || edits[i].kind != Edit::Kind::Pass) continue;
		const std::string_view s = text(tokens[i], source);
		std::string_view replacement;
		if (s == "gl_Vertex")                         replacement = "sf_Vertex";
		else if (s == "gl_MultiTexCoord0")            replacement = "sf_MultiTexCoord0";
		else if (s == "gl_Color")                     replacement = "sf_Color";
		else if (s == "gl_FrontColor")                replacement = "sf_color";
		else if (s == "gl_TexCoord")                  replacement = "sf_TexCoord";
		else if (s == "gl_ModelViewProjectionMatrix") replacement = "sf_ModelViewProjectionMatrix";
		else if (s == "gl_ModelViewMatrix")           replacement = "sf_modelview";
		else if (s == "gl_ProjectionMatrix")          replacement = "sf_projection";
		else if (s == "gl_TextureMatrix") {
			// Only the first texture unit exists: gl_TextureMatrix[0] -> sf_TextureMatrix
			if (i + 3 >= tokens.size() || !isPunctChar(tokens[i + 1], source, '[')
			    || text(tokens[i + 2], source) != "0" || !isPunctChar(tokens[i + 3], source, ']')) continue;
			edits[i + 1].kind = edits[i + 2].kind = edits[i + 3].kind = Edit::Kind::Skip;
			replacement = "sf_TextureMatrix";
		}
		if (replacement.empty()) continue;
		edits[i].kind        = Edit::Kind::Replace;
		edits[i].replacement = std::string(replacement);
	}

	const bool raylib = iface == VertexInterface::Raylib;
	if (raylib) stripHeaderVaryingRedeclarations(tokens, source, edits);
	return std::string(kProcessedMarker) + (raylib ? kRaylibLegacyVertexHeader : kLegacyVertexHeader)
	     + emit(tokens, source, edits) + (raylib ? kRaylibLegacyVertexMain : kLegacyVertexMain);
}

std::string resolveGlEsConditionals(const std::string& source, bool glEs) {
	if (source.find("GL_ES") == std::string::npos) return source;

	// Kind of the open conditional: a GL_ES test (resolved here), or any
	// other one (left to the GLSL compiler).
	struct Frame { bool glEsTest; bool live; };
	std::vector<Frame> frames;
	auto allLive = [&frames] {
		return std::all_of(frames.begin(), frames.end(), [](const Frame& f) { return f.live; });
	};
	auto directiveIs = [](std::string_view line, std::string_view name) {
		std::size_t i = line.find('#') + 1;
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
		if (line.compare(i, name.size(), name) != 0) return std::string_view::npos;
		return i + name.size();
	};
	// Value of the GL_ES test of an #if / #ifdef / #ifndef line, if it is one.
	auto glEsTest = [&](std::string_view line) -> int {
		auto rest = [&](std::size_t from) {
			std::string out;
			for (char c : line.substr(from)) if (c != ' ' && c != '\t' && c != '\r' && c != '\n') out.push_back(c);
			return out;
		};
		if (auto at = directiveIs(line, "ifdef");  at != std::string_view::npos) return rest(at) == "GL_ES" ? 1 : -1;
		if (auto at = directiveIs(line, "ifndef"); at != std::string_view::npos) return rest(at) == "GL_ES" ? 0 : -1;
		if (auto at = directiveIs(line, "if");     at != std::string_view::npos) {
			const std::string cond = rest(at);
			if (cond == "defined(GL_ES)" || cond == "definedGL_ES") return 1;
			if (cond == "!defined(GL_ES)") return 0;
		}
		return -1;
	};

	std::string out;
	out.reserve(source.size());
	std::size_t pos = 0;
	while (pos < source.size()) {
		std::size_t eol = source.find('\n', pos);
		const std::size_t end = eol == std::string::npos ? source.size() : eol + 1;
		const std::string_view line(source.data() + pos, end - pos);
		pos = end;

		const std::size_t first = line.find_first_not_of(" \t");
		const bool directive = first != std::string_view::npos && line[first] == '#';
		if (directive) {
			if (directiveIs(line, "if") != std::string_view::npos) { // #if, #ifdef, #ifndef
				const int test = glEsTest(line);
				if (test >= 0) {
					frames.push_back({ true, (test == 1) == glEs });
					continue;
				}
				frames.push_back({ false, true });
			} else if (!frames.empty() && frames.back().glEsTest && directiveIs(line, "else") != std::string_view::npos) {
				frames.back().live = !frames.back().live;
				continue;
			} else if (!frames.empty() && directiveIs(line, "endif") != std::string_view::npos) {
				const bool resolved = frames.back().glEsTest;
				frames.pop_back();
				if (resolved) continue;
			}
		}
		if (allLive()) out.append(line);
	}
	return out;
}

std::string writeNpotVarying(const std::string& source) {
	const auto tokens = lex(source);
	if (!hasIdent(tokens, source, "v_texture_coordinates") || hasIdent(tokens, source, "v_factor_npot")) return source;
	const SymbolTable sym = discoverSymbols(tokens, source);

	std::vector<Edit> edits(tokens.size());
	bool hasMain = false;
	for (const auto& fn : sym.functions) {
		if (fn.name == "main") {
			edits[fn.nameTok].kind        = Edit::Kind::Replace;
			edits[fn.nameTok].replacement = "sf_author_main";
			hasMain = true;
		}
	}
	if (!hasMain) return source;

	std::string out = emit(tokens, source, edits) + "\nvarying vec2 v_factor_npot;\n";
	if (!hasIdent(tokens, source, "factor_npot")) out += "uniform vec2 factor_npot;\n";
	return out + kNpotVaryingMain;
}

std::string enforceEsStrictness(const std::string& source, std::vector<ShaderPreprocessor::UniformDefault>& defaults,
                                bool glslEs100) {
	const auto tokens = lex(source);
	const SymbolTable sym = discoverSymbols(tokens, source);
	std::vector<Edit> edits(tokens.size());
	stripUniformInitializers(tokens, source, edits, defaults);
	if (glslEs100) rewriteIntegerModulus(tokens, source, edits);
	promoteIntegerLiterals(tokens, source, edits);
	if (glslEs100) {
		// After the promotion: element expressions are emitted with its edits
		rewriteConstArrays(tokens, source, edits);
		addRoundPolyfill(tokens, source, sym, edits);
	}
	return emit(tokens, source, edits);
}

} // namespace cgss::glsl
