#include "glsl_symbols.h"

#include <algorithm>

namespace cgss::glsl {

namespace {

bool endsWith(std::string_view s, std::string_view suffix) {
	return s.size() >= suffix.size()
	    && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Push `name` into `out` if it's not already there. Keeps order of first
// appearance, matching the legacy regex-based discovery.
void pushUnique(std::vector<std::string>& out, std::string_view name) {
	if (std::find(out.begin(), out.end(), name) == out.end()) {
		out.emplace_back(name);
	}
}

// Parses a `#define NAME TARGET` directive into a MacroAlias when TARGET is a
// single identifier (optionally wrapped in balanced parens). Returns true on
// success, false for everything else (function-like macros, complex bodies,
// malformed inputs) so the caller silently skips.
//
// Strategy: re-lex the directive body (everything after the leading `#`) with
// the same lexer, then parse using TokenCursor. This keeps character-level
// handling — whitespace, identifier characters, comments — out of this file;
// it all happens inside lex().
bool tryParseDefineAlias(std::string_view directiveLine, MacroAlias& out) {
	if (directiveLine.empty() || directiveLine.front() != '#') return false;
	// Skip the '#'. lex() restarts at_line_start=true internally, which would
	// re-collapse the whole remaining text into one Preproc token if we left
	// the '#' in; slicing it off lets the inner pass tokenize normally.
	const std::string_view body = directiveLine.substr(1);
	const auto sub = lex(body);

	auto c = TokenCursor(sub, body).skipTrivia();
	if (c != "define") return false;
	const auto afterDefine = c.next();
	// "define" must be followed by trivia (separator between keyword and name).
	if (!afterDefine.isTrivia()) return false;

	c = afterDefine.skipTrivia();
	if (!c.isIdent()) return false;
	const auto name      = c.text();
	const auto afterName = c.next();

	// Function-like macros: '(' immediately after the name with no trivia.
	if (afterName.isPunct('(')) return false;

	const auto target = bareIdentBetween(afterName.skipTrivia());
	if (target.empty()) return false;

	out.name   = std::string(name);
	out.target = std::string(target);
	return true;
}

// Parses a "uniform [precision-qualifier] <type> <ident>[, <ident>]* ;" decl.
// The cursor `c` enters parked on the "uniform" Ident; the function returns a
// cursor advanced just past the terminating ';' (or to eof on a premature end,
// or to wherever the malformed shape stopped being recognizable). Records any
// sampler2D / vec2 _factor_npot declarators it finds along the way.
TokenCursor parseUniform(TokenCursor c, SymbolTable& sym) {
	c = c.next().skipTrivia();
	if (c.eof()) return c;

	// Optional precision qualifier.
	c = c.skipIdents("highp", "mediump", "lowp");
	if (!c.isIdent()) return c;

	const auto typeName    = c.text();
	const bool isSampler2D = (typeName == "sampler2D");
	const bool isVec2      = (typeName == "vec2");
	c = c.next().skipTrivia();

	// Walk the declarator list: <name> [= <expr>] (, <name> [= <expr>])* ;
	// Any unexpected token shape stops parsing without consuming further
	// declarations.
	while (!c.eof()) {
		if (!c.isIdent()) break;
		const auto varName = c.text();
		if (isSampler2D) {
			pushUnique(sym.topLevelSamplers, varName);
		} else if (isVec2 && endsWith(varName, "_factor_npot")) {
			pushUnique(sym.declaredFactorNpots, varName);
		}
		c = c.next().skipTrivia();

		// Optional "= <initializer>" — scan forward to the next ',' or ';'
		// at paren-depth 0 so we don't get confused by commas inside an
		// initializer expression.
		if (c.isPunct('=')) {
			c = c.next();
			int paren = 0;
			while (!c.eof()) {
				if (c.isPunct('(') || c.isPunct('[')) ++paren;
				else if (c.isPunct(')') || c.isPunct(']')) --paren;
				else if (paren == 0 && (c.isPunct(',') || c.isPunct(';'))) break;
				c = c.next();
			}
		}

		if (c.eof())      return c;
		if (c.isPunct(',')) { c = c.next().skipTrivia(); continue; }
		if (c.isPunct(';')) return c.next();
		break;
	}
	return c;
}

// Attempts to parse a function declaration starting at the return-type Ident.
// Handles both definitions (`<type> <name>(<params>) { ... }`) and forward
// declarations (`<type> <name>(<params>);`). On success records the function
// and returns a cursor parked at the '{' (definition) or just past the ';'
// (forward decl). On any failure returns the input cursor unchanged so the
// caller can fall through to a generic advance.
TokenCursor tryParseFunctionDecl(TokenCursor c, SymbolTable& sym) {
	const auto save    = c;
	const auto retType = c.text();

	c = c.next().skipTrivia();
	if (!c.isIdent()) return save;
	const auto fname             = c.text();
	const std::size_t nameTok    = c.index();

	c = c.next().skipTrivia();
	if (!c.isPunct('(')) return save;
	const std::size_t openParen = c.index();

	// Walk the parameter list with paren-depth tracking. For each parameter we
	// record:
	//   - If it's a sampler2D, the param's position and name token (for the
	//     signature/call-site rewriters in glsl_rewrite.cpp).
	//   - If it's `vec2 <name>_factor_npot`, the name — so the signature
	//     rewriter skips injecting a colliding companion parameter.
	std::vector<SamplerParam> samplerParams;
	std::vector<std::string>  existingFactorParams;
	std::size_t               paramStart = openParen + 1;
	std::size_t               paramIndex = 0;

	auto flushParam = [&](std::size_t paramEnd) {
		auto p = TokenCursor(c.stream(), c.source(), paramStart).skipTrivia();
		// Skip precision/storage qualifiers ahead of the type.
		p = p.skipIdents("highp", "mediump", "lowp", "in", "out", "inout", "const");
		if (p.index() >= paramEnd || !p.isIdent()) { ++paramIndex; return; }
		const auto typeName = p.text();
		p = p.next().skipTrivia();
		if (p.index() >= paramEnd || !p.isIdent()) { ++paramIndex; return; }
		const std::size_t paramNameTok = p.index();
		const auto        paramName    = p.text();
		if (typeName == "sampler2D") {
			SamplerParam sp;
			sp.name     = std::string(paramName);
			sp.position = paramIndex;
			sp.nameTok  = paramNameTok;
			samplerParams.push_back(std::move(sp));
		} else if (typeName == "vec2" && endsWith(paramName, "_factor_npot")) {
			existingFactorParams.emplace_back(paramName);
		}
		++paramIndex;
	};

	c = c.next();
	int paren = 1;
	while (!c.eof() && paren > 0) {
		if (c.isPunct('(')) {
			++paren;
		} else if (c.isPunct(')')) {
			--paren;
			if (paren == 0) {
				// Only flush a final parameter if there's actually something
				// (non-trivia) between paramStart and the close paren —
				// otherwise an empty `()` would record a phantom parameter.
				auto probe = TokenCursor(c.stream(), c.source(), paramStart).skipTrivia();
				if (probe.index() < c.index()) {
					flushParam(c.index());
				}
				break;
			}
		} else if (paren == 1 && c.isPunct(',')) {
			flushParam(c.index());
			paramStart = c.index() + 1;
		}
		c = c.next();
	}
	if (c.eof()) return save;

	const std::size_t closeParenTok = c.index();
	c = c.next().skipTrivia();
	if (c.eof()) return save;

	FunctionInfo fi;
	fi.name                 = std::string(fname);
	fi.samplerParams        = std::move(samplerParams);
	fi.existingFactorParams = std::move(existingFactorParams);
	fi.nameTok              = nameTok;
	fi.closeParenTok        = closeParenTok;

	if (c.isPunct('{')) {
		fi.isDefinition = true;
		fi.bodyOpenTok  = c.index();
		if (sym.firstDefinitionTok == SymbolTable::npos) {
			std::size_t start = save.index();
			const auto qualifier = TokenCursor(c.stream(), c.source(), start).prev();
			// `highp vec4 f()`: the qualifier belongs to the definition
			for (auto q = qualifier; q.index() < start && (q.isTrivia() || q == "highp" || q == "mediump" || q == "lowp");
			     q = q.prev()) {
				if (q.isIdent()) start = q.index();
				if (q.index() == 0) break;
			}
			sym.firstDefinitionTok = start;
		}
		sym.functions.push_back(std::move(fi));
		if (retType == "void" && fname == "main") {
			sym.voidMainTok = save.index();
		}
		return c;
	}
	if (c.isPunct(';')) {
		fi.isDefinition = false;
		sym.functions.push_back(std::move(fi));
		return c.next();
	}
	return save;
}

} // namespace

SymbolTable discoverSymbols(const std::vector<Token>& tokens, std::string_view source) {
	SymbolTable sym;
	int         depth = 0;
	auto        c     = TokenCursor(tokens, source);

	while (!c.eof()) {
		if (c == TokenKind::Preproc) {
			MacroAlias alias;
			if (tryParseDefineAlias(c.text(), alias)) {
				// Only record the first definition for a given name; a later
				// `#define` would shadow it but we don't track scopes.
				const bool dup = std::any_of(sym.macroAliases.begin(), sym.macroAliases.end(),
				                             [&](const MacroAlias& a) { return a.name == alias.name; });
				if (!dup) sym.macroAliases.push_back(std::move(alias));
			}
			c = c.next();
			continue;
		}
		if (c.isPunct('{'))                       { ++depth;             c = c.next(); continue; }
		if (c.isPunct('}'))                       { if (depth) --depth;  c = c.next(); continue; }

		if (depth == 0 && c.isIdent()) {
			if (c == "uniform") {
				c = parseUniform(c, sym);
				continue;
			}
			// `const vec2 <sampler>_factor_npot = ...;` declares the factor as
			// much as a uniform does (a shader's desktop branch: a padded-
			// coordinates PSDK shader keeps its own maths, 1.0 off GL ES).
			if (c == "const") {
				auto type = c.next().skipTrivia().skipIdents("highp", "mediump", "lowp");
				if (type.isIdent() && type.text() == "vec2") {
					auto name = type.next().skipTrivia();
					if (name.isIdent() && endsWith(name.text(), "_factor_npot")) {
						pushUnique(sym.declaredFactorNpots, name.text());
					}
				}
				c = c.next();
				continue;
			}
			// Try to interpret c as the return type of a function declaration
			// (definition or forward decl). On success this advances to the
			// body's '{' (definition — the '{' bumps `depth` on the next
			// iteration) or just past the ';' (forward decl).
			auto next = tryParseFunctionDecl(c, sym);
			if (next.index() != c.index()) { c = next; continue; }
		}
		c = c.next();
	}

	// Second pass: fill bodyCloseTok by matching braces from each bodyOpenTok.
	// Done separately so the main pass stays linear (no nested scans). Forward
	// declarations have no body and are skipped.
	for (auto& fi : sym.functions) {
		if (!fi.isDefinition) continue;
		int  d = 1;
		auto k = TokenCursor(tokens, source, fi.bodyOpenTok + 1);
		while (!k.eof() && d > 0) {
			if      (k.isPunct('{')) ++d;
			else if (k.isPunct('}')) --d;
			if (d == 0) { fi.bodyCloseTok = k.index(); break; }
			k = k.next();
		}
	}
	return sym;
}

} // namespace cgss::glsl
