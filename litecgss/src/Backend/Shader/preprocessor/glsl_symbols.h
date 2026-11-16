#pragma once

// Symbol discovery pass over a tokenized GLSL source. The output is the small
// set of facts the rewriter needs:
//   - which top-level identifiers are `sampler2D` uniforms,
//   - which `<name>_factor_npot` uniforms the author already declared (so we
//     don't redeclare them),
//   - where each function definition starts/ends, with its `sampler2D` parameter
//     names (parameter-name knowledge is recorded now even though v1 of the
//     rewriter only treats it as "not a top-level sampler — skip the wrap").
//
// This pass does NOT validate GLSL. Malformed declarations are simply not
// recognized; nothing is rejected, nothing throws.

#include "glsl_lexer.h"

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace cgss::glsl {

struct SamplerParam {
	std::string name;
	// 0-indexed position in the parameter list. The call-site rewriter uses
	// this to locate the matching argument slot at each call.
	std::size_t position = 0;
	// Token index of the Ident holding this parameter's name. The signature
	// rewriter uses this as an anchor to inject the companion
	// `vec2 <name>_factor_npot` parameter just after it.
	std::size_t nameTok  = 0;
};

struct FunctionInfo {
	std::string                name;
	std::vector<SamplerParam>  samplerParams;
	// Names of `vec2 <something>_factor_npot` parameters the author already
	// wrote into the signature. Used by the signature-injection pass to skip
	// adding a companion parameter that would collide.
	std::vector<std::string>   existingFactorParams;
	// Token index of the function-name Ident. Lets the call-site rewriter
	// distinguish the declaration's own name from a call to it.
	std::size_t                nameTok      = 0;
	// True for `<type> <name>(<params>) { ... }`; false for the bare
	// `<type> <name>(<params>);` forward declaration shape.
	bool                       isDefinition = true;
	// Body span. bodyOpenTok/bodyCloseTok are both 0 for forward decls.
	std::size_t                bodyOpenTok  = 0;
	std::size_t                bodyCloseTok = 0;
	// Token index of the closing ')' of the parameter list.
	std::size_t                closeParenTok = 0;
};

// A simple alias macro of the form `#define NAME TARGET` where TARGET is a
// single identifier (optionally wrapped in parens). Function-like macros and
// multi-token bodies are deliberately not tracked — they're rare in shader
// source and the rewriter would have to fully expand them, which is outside
// the scope of the preprocessor. The single-identifier alias case is the
// useful one: `#define MY_TEXTURE rubyTexture` lets the rewriter still find
// the underlying sampler when forming `<sampler>_factor_npot`.
struct MacroAlias {
	std::string name;
	std::string target;
};

struct SymbolTable {
	std::vector<std::string>  topLevelSamplers;
	std::vector<std::string>  declaredFactorNpots;
	std::vector<FunctionInfo> functions;
	std::vector<MacroAlias>   macroAliases;
	// Token index of the "void" keyword that introduces "void main(". Stays
	// at npos if main() is not present (some include-style snippets).
	static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();
	std::size_t voidMainTok = npos;
	// Token index where the first function definition starts (its return
	// type, or the precision qualifier before it). npos if there is none.
	std::size_t firstDefinitionTok = npos;
};

SymbolTable discoverSymbols(const std::vector<Token>& tokens, std::string_view source);

} // namespace cgss::glsl
