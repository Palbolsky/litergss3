#pragma once

// Token-based rewrite pass. Produces the body of the processed shader from
// the tokens + symbol table; the higher ShaderPreprocessor layer still
// handles the prepended precision/varying headers, processed marker, and
// optional #version directive.
//
// The transformations applied here mirror the legacy regex pipeline:
//   - strip author-side `varying vec2 v_factor_npot;` and
//     `const vec2 v_factor_npot = vec2(...);` redeclarations,
//   - rewrite `gl_TexCoord[0]`, `gl_TexCoord[1]`, `gl_Color` to their modern
//     varying equivalents,
//   - wrap `texture2D(s, uv)` (and the rarer `texture2DProj` / `texture2DLod`)
//     into `texture2D(s, (uv) * s_factor_npot)` when `s` is a top-level sampler
//     and the UV does not already mention any *_factor_npot identifier,
//   - inject the auto-generated per-sampler NPOT factor declaration block
//     immediately before `void main(` (skipping any sampler whose factor uniform
//     the author already declared by hand).

#include "glsl_lexer.h"
#include "glsl_symbols.h"

#include <string>
#include <string_view>
#include <vector>

namespace cgss::glsl {

// Apply all token-level rewrites and emit the rewritten source. Whitespace
// and comments outside rewritten regions pass through verbatim.
// `rewriteBuiltins`: also rewrite gl_TexCoord / gl_Color (targets on which
// the fixed-pipeline built-ins aren't fed, see ShaderPreprocessor::Target).
std::string rewriteAndEmit(const std::vector<Token>& tokens,
                           std::string_view source,
                           const SymbolTable& sym,
                           bool rewriteBuiltins = true);

// Exposed for unit testing. Builds the same block the rewriter would inject
// before "void main(" for the given symbol table. Returns "" when there are
// no samplers to declare factors for.
std::string buildFactorDeclarations(const SymbolTable& sym);

} // namespace cgss::glsl
