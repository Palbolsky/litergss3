#include "glsl_rewrite.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>

namespace cgss::glsl {

namespace {

struct TokenAction {
	enum class Kind : std::uint8_t { Pass, Replace, Skip };
	Kind        kind = Kind::Pass;
	std::string replacement;
	std::string insertBefore;
};

bool endsWith(std::string_view s, std::string_view suffix) {
	return s.size() >= suffix.size()
	    && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Skip only horizontal whitespace (spaces/tabs), not newlines or comments.
// Used where the legacy regex required adjacency on the same line (e.g. the
// "(" after a texture2D call).
std::size_t skipHorizontalWs(const std::vector<Token>& tokens, std::size_t i) {
	while (i < tokens.size() && tokens[i].kind == TokenKind::Whitespace) ++i;
	return i;
}

// True if any Ident in [start, end) has text ending in "_factor_npot". The
// rewriter uses this as the skip-condition for the texture2D auto-wrap: if
// the author has already woven a factor into the UV math, leave the call alone.
bool rangeMentionsFactorNpot(const std::vector<Token>& tokens, std::string_view source, std::size_t start, std::size_t end) {
	static constexpr std::string_view kSuffix = "_factor_npot";
	for (std::size_t i = start; i < end; ++i) {
		if (tokens[i].kind != TokenKind::Ident) continue;
		const std::string_view s = text(tokens[i], source);
		if (s.size() > kSuffix.size() && endsWith(s, kSuffix)) {
			return true;
		}
	}
	return false;
}

// Resolves `name` through the macro alias chain, bounded to avoid pathological
// cyclic `#define` chains. The terminal name is returned (which is also the
// input when no alias matches).
std::string_view resolveAlias(const SymbolTable& sym, std::string_view name) {
	for (int hop = 0; hop < 16; ++hop) {
		auto it = std::find_if(sym.macroAliases.begin(), sym.macroAliases.end(),
		                       [&](const MacroAlias& a) { return a.name == name; });
		if (it == sym.macroAliases.end()) return name;
		name = it->target;
	}
	return name;
}

// Returns the FunctionInfo whose body contains `tokIdx`, or nullptr if the
// position is at file scope. GLSL forbids nested function definitions, so at
// most one body can match. Forward declarations have no body and are skipped.
const FunctionInfo* findEnclosingFunction(const SymbolTable& sym, std::size_t tokIdx) {
	for (const auto& fn : sym.functions) {
		if (!fn.isDefinition) continue;
		if (fn.bodyOpenTok < tokIdx && tokIdx < fn.bodyCloseTok) {
			return &fn;
		}
	}
	return nullptr;
}

// True iff `ident` resolves to a sampler the rewriter knows about — either a
// top-level uniform or the enclosing function's sampler parameter. The check
// is performed AFTER resolving any `#define` alias chain, so an alias macro
// pointing at a real sampler still counts.
bool isSamplerInScope(std::string_view ident, const SymbolTable& sym, const FunctionInfo* enclosing) {
	const std::string_view resolved = resolveAlias(sym, ident);
	if (std::find(sym.topLevelSamplers.begin(), sym.topLevelSamplers.end(), resolved) != sym.topLevelSamplers.end()) {
		return true;
	}
	if (enclosing) {
		for (const auto& sp : enclosing->samplerParams) {
			if (sp.name == resolved) return true;
		}
	}
	return false;
}

// Returns the token indices of the per-argument terminators (',' separators
// plus the closing ')'), in left-to-right order, for the argument list opened
// by tokens[openParenTok] == '('. The number of args is exactly the size of
// the returned vector. Returns an empty vector on malformed input
// (unmatched paren).
std::vector<std::size_t> argEndTokens(const std::vector<Token>& tokens, std::string_view source, std::size_t openParenTok) {
	std::vector<std::size_t> ends;
	int paren = 1;
	for (std::size_t k = openParenTok + 1; k < tokens.size(); ++k) {
		if (isPunctChar(tokens[k], source, '(') || isPunctChar(tokens[k], source, '[')) {
			++paren;
		} else if (isPunctChar(tokens[k], source, ')') || isPunctChar(tokens[k], source, ']')) {
			--paren;
			if (paren == 0) {
				ends.push_back(k);
				return ends;
			}
		} else if (paren == 1 && isPunctChar(tokens[k], source, ',')) {
			ends.push_back(k);
		}
	}
	return {};
}


// Picks the SFML-bound "primary" sampler — by convention "texture" or the
// RGSS-era "rubyTexture", falling back to the first declared sampler. SFML
// keeps v_factor_npot synced with that texture's NPOT factor; secondary
// samplers each get their own uniform pushed via setShaderUniform.
std::string_view detectPrimarySampler(const std::vector<std::string>& samplers) {
	for (const auto& candidate : { std::string_view("texture"), std::string_view("rubyTexture") }) {
		auto it = std::find(samplers.begin(), samplers.end(), candidate);
		if (it != samplers.end()) {
			return *it;
		}
	}
	return samplers.front();
}

// Marks tokens belonging to author-side `varying vec2 v_factor_npot;` or
// `const vec2 v_factor_npot = vec2(...);` redeclarations as Skip, together
// with any leading horizontal whitespace and the line's trailing newline.
// The legacy regex strips whole lines; we replicate that line removal so the
// output doesn't gain stray blank lines.
void stripFactorNpotRedeclarations(const std::vector<Token>& tokens, std::string_view source, std::vector<TokenAction>& actions) {
	auto markSkip = [&](std::size_t from, std::size_t to) {
		for (std::size_t k = from; k < to; ++k) {
			actions[k].kind = TokenAction::Kind::Skip;
		}
	};

	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident) continue;
		const std::string_view kw = text(tokens[i], source);
		const bool isVarying = (kw == "varying");
		const bool isConst   = (kw == "const");
		if (!isVarying && !isConst) continue;

		std::size_t j = skipTrivia(tokens, i + 1);
		if (j >= tokens.size() || tokens[j].kind != TokenKind::Ident || text(tokens[j], source) != "vec2") continue;
		j = skipTrivia(tokens, j + 1);
		if (j >= tokens.size() || tokens[j].kind != TokenKind::Ident || text(tokens[j], source) != "v_factor_npot") continue;
		j = skipTrivia(tokens, j + 1);

		if (isVarying) {
			if (j >= tokens.size() || !isPunctChar(tokens[j], source, ';')) continue;
		} else {
			// `const vec2 v_factor_npot = vec2 ( ... ) ;`
			if (j >= tokens.size() || !isPunctChar(tokens[j], source, '=')) continue;
			j = skipTrivia(tokens, j + 1);
			if (j >= tokens.size() || tokens[j].kind != TokenKind::Ident || text(tokens[j], source) != "vec2") continue;
			j = skipTrivia(tokens, j + 1);
			if (j >= tokens.size() || !isPunctChar(tokens[j], source, '(')) continue;
			int paren = 1;
			++j;
			while (j < tokens.size() && paren > 0) {
				if (isPunctChar(tokens[j], source, '(')) ++paren;
				else if (isPunctChar(tokens[j], source, ')')) --paren;
				++j;
			}
			j = skipTrivia(tokens, j);
			if (j >= tokens.size() || !isPunctChar(tokens[j], source, ';')) continue;
		}

		std::size_t from = i;
		while (from > 0 && tokens[from - 1].kind == TokenKind::Whitespace) --from;
		std::size_t to = j + 1;
		if (to < tokens.size() && tokens[to].kind == TokenKind::Newline) ++to;
		markSkip(from, to);
	}
}

// True when the shader still uses v_factor_npot once its own redeclarations
// are stripped: it was written for texture coordinates over the padded
// storage of an NPOT texture (GL ES as SFML ES gives them, without this
// preprocessor) and divides by the factor itself where it needs logical ones.
bool usesPaddedCoordinates(const std::vector<Token>& tokens, std::string_view source, const std::vector<TokenAction>& actions) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (actions[i].kind == TokenAction::Kind::Pass && tokens[i].kind == TokenKind::Ident
		    && text(tokens[i], source) == "v_factor_npot") {
			return true;
		}
	}
	return false;
}

// Rewrites gl_TexCoord[0|1] and gl_Color. Requires direct adjacency of
// `gl_TexCoord` `[` `<digit>` `]` (no trivia in between) so output is
// byte-stable against the legacy regex on shaders written without odd spacing.
// A shader using padded coordinates gets them as they are.
void rewriteGlVariables(const std::vector<Token>& tokens, std::string_view source, std::vector<TokenAction>& actions, bool padded) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (actions[i].kind != TokenAction::Kind::Pass) continue;
		if (tokens[i].kind != TokenKind::Ident) continue;
		const std::string_view s = text(tokens[i], source);
		if (s == "gl_Color") {
			actions[i].kind        = TokenAction::Kind::Replace;
			actions[i].replacement = "sf_color";
			continue;
		}
		if (s != "gl_TexCoord") continue;

		if (i + 3 >= tokens.size()) continue;
		if (!isPunctChar(tokens[i + 1], source, '[')) continue;
		if (tokens[i + 2].kind != TokenKind::Number) continue;
		if (!isPunctChar(tokens[i + 3], source, ']')) continue;

		const std::string_view idx = text(tokens[i + 2], source);
		std::string_view replacement;
		if (idx == "0")      replacement = padded ? "v_texture_coordinates" : "(v_texture_coordinates / v_factor_npot)";
		else if (idx == "1") replacement = padded ? "v_inv_texture_coordinates" : "(v_inv_texture_coordinates / v_factor_npot)";
		else                 continue;

		actions[i].kind        = TokenAction::Kind::Replace;
		actions[i].replacement = std::string(replacement);
		actions[i + 1].kind    = TokenAction::Kind::Skip;
		actions[i + 2].kind    = TokenAction::Kind::Skip;
		actions[i + 3].kind    = TokenAction::Kind::Skip;
	}
}

// Wraps texture2D / texture2DProj / texture2DLod calls so the UV argument is
// multiplied by `<sampler>_factor_npot`. The sampler may be either a top-level
// uniform (factor declared by injectFactorDeclarations) or the enclosing
// function's sampler parameter (factor injected by injectFunctionSignatures
// below). Skip conditions: sampler arg isn't a single identifier; sampler
// isn't a known sampler in scope; UV expression already mentions any
// *_factor_npot identifier (author-supplied factor math).
void wrapTexture2DCalls(const std::vector<Token>& tokens, std::string_view source, const SymbolTable& sym, std::vector<TokenAction>& actions) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident) continue;
		if (actions[i].kind == TokenAction::Kind::Skip) continue;
		const std::string_view fname = text(tokens[i], source);
		if (fname != "texture2D" && fname != "texture2DProj" && fname != "texture2DLod") continue;

		// The legacy regex required '(' on the same line (only horizontal
		// whitespace between `texture2D` and '('). Match that for byte-stable
		// output on conventional shaders.
		std::size_t j = skipHorizontalWs(tokens, i + 1);
		if (j >= tokens.size() || !isPunctChar(tokens[j], source, '(')) continue;
		const std::size_t openParen = j;

		int paren = 1;
		std::size_t commaTok   = SymbolTable::npos;
		std::size_t closeParen = SymbolTable::npos;
		for (std::size_t k = j + 1; k < tokens.size(); ++k) {
			if (isPunctChar(tokens[k], source, '(')) {
				++paren;
			} else if (isPunctChar(tokens[k], source, ')')) {
				--paren;
				if (paren == 0) {
					closeParen = k;
					break;
				}
			} else if (paren == 1 && isPunctChar(tokens[k], source, ',') && commaTok == SymbolTable::npos) {
				commaTok = k;
			}
		}
		if (commaTok == SymbolTable::npos || closeParen == SymbolTable::npos) continue;

		// Sampler arg must reduce to a bare identifier (paren-peeling allowed).
		const std::string_view sampler = bareIdentBetween(
			TokenCursor(tokens, source, openParen + 1),
			TokenCursor(tokens, source, commaTok));
		if (sampler.empty()) continue;

		const FunctionInfo* enclosing = findEnclosingFunction(sym, i);
		if (!isSamplerInScope(sampler, sym, enclosing)) continue;

		if (rangeMentionsFactorNpot(tokens, source, commaTok + 1, closeParen)) continue;

		const std::size_t uv0 = skipTrivia(tokens, commaTok + 1);
		if (uv0 >= closeParen) continue;

		// Use the resolved identifier when forming the factor name: an alias
		// macro like `#define TEX foo` still needs to multiply by `foo_factor_npot`.
		const std::string_view factorBase = resolveAlias(sym, sampler);
		actions[uv0].insertBefore        += "(";
		actions[closeParen].insertBefore += ") * ";
		actions[closeParen].insertBefore += factorBase;
		actions[closeParen].insertBefore += "_factor_npot";
	}
}

// Signature-injection pass: for each function (definition or forward decl)
// that has sampler2D parameters, inject a companion `vec2 <name>_factor_npot`
// parameter immediately after each sampler. Skipped when the author already
// supplied a matching factor parameter — that allows shaders to thread the
// factor manually without our pass adding a duplicate.
void injectFunctionSignatures(const std::vector<Token>& tokens, std::string_view source, const SymbolTable& sym, std::vector<TokenAction>& actions) {
	for (const auto& fn : sym.functions) {
		if (fn.samplerParams.empty()) continue;

		// Compute the function's open paren (skipping trivia after the name
		// token). Then build the per-arg terminator list once and reuse it.
		const std::size_t openParen = skipTrivia(tokens, fn.nameTok + 1);
		if (openParen >= tokens.size() || !isPunctChar(tokens[openParen], source, '(')) continue;
		const auto ends = argEndTokens(tokens, source, openParen);
		if (ends.empty()) continue;

		for (const auto& sp : fn.samplerParams) {
			const std::string factorName = sp.name + "_factor_npot";
			if (std::find(fn.existingFactorParams.begin(), fn.existingFactorParams.end(), factorName) != fn.existingFactorParams.end()) {
				continue;
			}
			if (sp.position >= ends.size()) continue;
			const std::size_t paramEnd = ends[sp.position];
			actions[paramEnd].insertBefore += ", vec2 ";
			actions[paramEnd].insertBefore += factorName;
		}
	}
}

// True iff token index `i` is the name token of any function declaration —
// i.e. the rewriter is sitting on a function's own name, not on a call to it.
bool isFunctionNameDeclToken(const SymbolTable& sym, std::size_t i) {
	for (const auto& fn : sym.functions) {
		if (fn.nameTok == i) return true;
	}
	return false;
}

// Looks up the first FunctionInfo with this name. Forward decls and the
// definition share signatures (GLSL requires that), so any entry is fine for
// reading the sampler-parameter layout.
const FunctionInfo* findFunctionByName(const SymbolTable& sym, std::string_view name) {
	for (const auto& fn : sym.functions) {
		if (fn.name == name) return &fn;
	}
	return nullptr;
}

// Call-site injection pass: for each call to a function that declares
// sampler2D parameters, inject a companion `<arg>_factor_npot` argument at
// the matching position(s). The argument is injected only when the matching
// call argument is a bare identifier resolving to a sampler in scope —
// anything more complex (a function call, a parenthesized expression) is
// skipped because we can't statically determine which factor uniform to
// pass.
void injectCallSiteArguments(const std::vector<Token>& tokens, std::string_view source, const SymbolTable& sym, std::vector<TokenAction>& actions) {
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind != TokenKind::Ident) continue;
		if (isFunctionNameDeclToken(sym, i)) continue;

		const FunctionInfo* fn = findFunctionByName(sym, text(tokens[i], source));
		if (!fn || fn->samplerParams.empty()) continue;

		const std::size_t openParen = skipHorizontalWs(tokens, i + 1);
		if (openParen >= tokens.size() || !isPunctChar(tokens[openParen], source, '(')) continue;

		const auto ends = argEndTokens(tokens, source, openParen);
		if (ends.empty()) continue;

		const FunctionInfo* enclosing = findEnclosingFunction(sym, i);

		for (const auto& sp : fn->samplerParams) {
			const std::string factorName = sp.name + "_factor_npot";
			if (std::find(fn->existingFactorParams.begin(), fn->existingFactorParams.end(), factorName) != fn->existingFactorParams.end()) {
				continue;
			}
			if (sp.position >= ends.size()) continue;

			const std::size_t argEnd  = ends[sp.position];
			const std::size_t argStart = (sp.position == 0) ? openParen + 1 : ends[sp.position - 1] + 1;
			const std::string_view argIdent = bareIdentBetween(
				TokenCursor(tokens, source, argStart),
				TokenCursor(tokens, source, argEnd));
			if (argIdent.empty()) continue;
			if (!isSamplerInScope(argIdent, sym, enclosing)) continue;

			// Resolve aliases when forming the factor identifier so an
			// alias like `#define TEX foo` produces `foo_factor_npot`.
			const std::string_view factorBase = resolveAlias(sym, argIdent);
			actions[argEnd].insertBefore += ", ";
			actions[argEnd].insertBefore += factorBase;
			actions[argEnd].insertBefore += "_factor_npot";
		}
	}
}

} // namespace

std::string buildFactorDeclarations(const SymbolTable& sym) {
	if (sym.topLevelSamplers.empty()) return std::string();

	const std::string_view primary = detectPrimarySampler(sym.topLevelSamplers);
	std::ostringstream out;
	out << "// --- per-sampler NPOT factors (auto-generated) ---\n";
	for (const auto& name : sym.topLevelSamplers) {
		const std::string token = name + "_factor_npot";
		if (std::find(sym.declaredFactorNpots.begin(), sym.declaredFactorNpots.end(), token) != sym.declaredFactorNpots.end()) {
			continue;
		}
		if (name == primary) {
			out << "#define " << name << "_factor_npot v_factor_npot\n";
		} else {
			out << "uniform vec2 " << name << "_factor_npot;\n";
		}
	}
	out << "// -------------------------------------------------\n";
	return out.str();
}

std::string rewriteAndEmit(const std::vector<Token>& tokens, std::string_view source, const SymbolTable& sym, bool rewriteBuiltins) {
	std::vector<TokenAction> actions(tokens.size());

	stripFactorNpotRedeclarations(tokens, source, actions);
	// A shader compensating the NPOT padding itself (PSDK Technical Demo's
	// title button and map overlays) keeps padded coordinates: normalizing
	// them too divided its coordinates twice. Its texture2D lookups are
	// already in padded space.
	const bool padded = usesPaddedCoordinates(tokens, source, actions);
	// Inject companion `vec2 <name>_factor_npot` parameters into function
	// signatures (definitions and forward decls) and matching arguments at
	// each call site. Both are pure insertions on signature/call-site
	// separator tokens, so they don't interact with the rewrites below.
	injectFunctionSignatures(tokens, source, sym, actions);
	injectCallSiteArguments(tokens, source, sym, actions);
	// Order matters: wrap texture2D BEFORE rewriting gl_TexCoord. The wrap's
	// skip-check sees any *_factor_npot identifier in the UV and bails — which
	// is the right behavior for author-supplied factor math but would also
	// fire on the gl_TexCoord rewrite's injected `v_factor_npot` if we did
	// that pass first.
	if (!padded) wrapTexture2DCalls(tokens, source, sym, actions);
	if (rewriteBuiltins) rewriteGlVariables(tokens, source, actions, padded);

	// Before the first function, not before main(): helper functions defined
	// ahead of main() sample the top-level samplers too.
	if (sym.voidMainTok != SymbolTable::npos) {
		const std::string decls = buildFactorDeclarations(sym);
		if (!decls.empty()) {
			const std::size_t at = std::min(sym.firstDefinitionTok, sym.voidMainTok);
			actions[at].insertBefore = decls + actions[at].insertBefore;
		}
	}

	std::string out;
	out.reserve(source.size() + 256);
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		const auto& a = actions[i];
		if (!a.insertBefore.empty()) {
			out.append(a.insertBefore);
		}
		switch (a.kind) {
			case TokenAction::Kind::Pass:
				out.append(text(tokens[i], source));
				break;
			case TokenAction::Kind::Replace:
				out.append(a.replacement);
				break;
			case TokenAction::Kind::Skip:
				break;
		}
	}
	return out;
}

} // namespace cgss::glsl
