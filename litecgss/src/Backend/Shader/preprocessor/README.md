# LiteCGSS shader preprocessor (lexer-based pipeline)

Token-based GLSL preprocessor for handling NPOT texture sampling and
fixed-pipeline variable rewrites. The public entry point is
`cgss::ShaderPreprocessor::process` in `../ShaderPreprocessor.{h,cpp}`;
everything in this directory is its implementation.

## Files

| File              | Role                                                                                      |
| ----------------- | ----------------------------------------------------------------------------------------- |
| `glsl_lexer.re`   | re2c source for the tokenizer. Hand-edited; generates `glsl_lexer.cpp`.                   |
| `glsl_lexer.cpp`  | **Generated.** Committed so consumers don't need re2c installed. Do not hand-edit.        |
| `glsl_lexer.h`    | Token kinds (`Ident`, `Number`, `Punct`, comments, preproc, etc.) and the `lex()` entry.  |
| `glsl_symbols.{h,cpp}` | Discovery pass: top-level samplers, existing `_factor_npot` decls, function defs.    |
| `glsl_rewrite.{h,cpp}` | Rewrite pass: strip author redecls, wrap `texture2D`, replace `gl_TexCoord`/`gl_Color`. |
| `glsl_es.{h,cpp}` | GL ES only: desktop `#version`, legacy vertex shaders, GLSL ES 1.00 strictness (see below). |
| `glsl_330.{h,cpp}` | raylib only: GLSL 1.x -> GLSL 330 core syntax, the last step of `Target::RaylibGlsl330`. |

## Targets

Ported from LiteCGSS (SFML) at 2c9e158, where the GL ES rewrite was selected
at compile time (`LITECGSS_USE_FIXED_PIPELINE`). Here the graphics backend
reports a `ShaderPreprocessor::Target` (`Ops::shader_preprocessor_target()`)
and `RenderStatesData` passes it on every load:

| Target                 | Backend                          | Fixed-pipeline built-ins | Language   |
| ---------------------- | -------------------------------- | ------------------------ | ---------- |
| `DesktopFixedPipeline` | SFML (`LITECGSS_USE_FIXED_PIPELINE`, default) | kept     | as written |
| `SfmlEs100`            | SFML ES (SFML-opengl-es2 fork)   | rewritten on SFML ES's attributes | GLSL ES 1.00 |
| `RaylibGlsl330`        | raylib, desktop GL 3.3 / 4.3     | rewritten on raylib's attributes (`#ifdef GL_ES` branches resolved first) | GLSL 330 core |

Golden files of the raylib target live in
`test/src/projects/ShaderPreprocessor/cases/raylib/`.

## Regenerating the lexer

After editing `glsl_lexer.re`, regenerate the `.cpp`:

```sh
cmake --build <build-dir> --target regen_glsl_lexer
```

Or call re2c directly with the same flags the CMake target uses:

```sh
re2c -W --no-generation-date -o glsl_lexer.cpp glsl_lexer.re
```

Then commit both the `.re` and `.cpp` together. `re2c >= 3.0` is required for
the `re2c:eof = 0` sentinel mode used by `lex()`. Ubuntu 24.04 packages `re2c 3.1`
which is sufficient.

The `regen_glsl_lexer` target is omitted (with a status message) when re2c is
not on `PATH` at configure time, so production builds without re2c installed
still work — they consume the committed `.cpp` directly.

## Pipeline overview

`ShaderPreprocessor::process(source, Fragment)` runs:

1. **Idempotency check.** If the source begins with the processed marker, skip
   everything except the optional `#version` prepend.
2. **Lex** the source into a token vector (each token records its byte offset
   and length).
3. **Discover** top-level `uniform sampler2D <name>;` declarations
   (multi-declarator and precision-qualified variants supported), author-side
   `uniform vec2 <name>_factor_npot;` declarations, and function definitions
   with their `sampler2D` parameter names.
4. **Rewrite** the token stream:
   - Strip author redeclarations of `varying vec2 v_factor_npot;` /
     `const vec2 v_factor_npot = vec2(...);` (line-level removal).
   - **Inject** companion `vec2 <name>_factor_npot` parameters into every
     function signature (definition + matching forward declaration) that has
     `sampler2D` parameters. Skipped per-sampler when the author already
     supplied a matching factor parameter.
   - **Inject** companion `<arg>_factor_npot` arguments at every call site
     of a function with sampler parameters. The matching factor identifier
     resolves to either a top-level sampler's auto-declared factor uniform
     or the enclosing function's injected parameter (so factors thread
     correctly through chained helpers). Calls whose sampler argument isn't
     a bare identifier (function calls, parenthesized expressions) are
     skipped — we can't statically determine the factor.
   - Wrap `texture2D(s, uv)` / `texture2DProj` / `texture2DLod` to
     `texture2D(s, (uv) * s_factor_npot)` when `s` is a sampler in scope
     (top-level uniform or enclosing function's sampler param) **and** the
     UV expression doesn't already mention any `*_factor_npot` identifier.
   - Replace `gl_TexCoord[0]` / `gl_TexCoord[1]` / `gl_Color` with their
     modern varying equivalents.
   - Inject the per-sampler `_factor_npot` declaration block immediately
     before the first function definition (`void main(`, or a helper
     defined ahead of it: helpers sample the top-level samplers too).
     Primary sampler (by name convention: `texture` or `rubyTexture`, else
     the first declared) gets a `#define` against `v_factor_npot`;
     secondaries get their own `uniform vec2` declarations.
5. **Emit** the rewritten string. Whitespace and comments outside rewritten
   regions pass through verbatim.
6. **Prepend** the precision block, the GL_ES varying header, and the processed
   marker.
7. **Prepend** the `#version` directive if one was requested and the source
   didn't supply one.

## GL ES adaptation of desktop shaders

Built without `LITECGSS_USE_FIXED_PIPELINE` (GL ES), shaders written for the
desktop fixed pipeline — games made with an older PSDK, which prepends
`#version 130` to every shader — are made GLSL ES 1.00 (`glsl_es.cpp`):

- A desktop `#version` (`110`, `120 compatibility`, `130`...) becomes
  `#version 100`; `100` and `300 es` stay.
- **Legacy vertex shaders** (using `gl_Vertex`, `gl_MultiTexCoord0`,
  `gl_TextureMatrix[0]`, `gl_ModelViewProjectionMatrix`, `gl_Color`,
  `gl_FrontColor`, `gl_TexCoord`) get SFML ES's attributes and uniforms and
  the fragment stage's varyings. The built-ins become globals (`sf_Vertex`,
  `sf_TexCoord[2]`...), the shader's `main` is renamed `sf_legacy_main`, and
  a new `main` fills the globals, calls it, then writes the varyings. Its
  `gl_TexCoord` and `gl_TextureMatrix[0]` leave the NPOT padding out, like on
  desktop (`1.0 - gl_TexCoord[0].y` is 0 at the bottom of the image), and the
  new `main` gives the varyings their padded coordinates back. A vertex
  shader mentioning `GL_ES` or declaring attributes is left alone, unless it
  writes `v_texture_coordinates` but not `v_factor_npot`, which the fragment
  rewrite reads and GL ES refuses to link without: its `main` is renamed
  `sf_author_main` and a new `main` calls it, then writes the varying.
- A fragment-only shader is paired with `defaultVertexShader()` (GL ES can't
  link a program without a vertex shader).
- **Uniform initializers** are removed; constant ones (numbers, booleans,
  constructors of numbers) are reported by `processDetailed` and set by
  `RenderStatesData` once the shader is linked.
- **Integer literals** become float literals next to an operand known to be
  a float (declared float/vec/mat, float literal, float built-in or function
  result) or as a whole argument of a float built-in. Scopes aren't tracked:
  a name declared float anywhere counts as float.
- **`%`** becomes `int(mod(float(a), float(b)))`, **`round()`** gets
  overloads.
- A **file-scope const array** (`const vec4 C[2] = vec4[](a, b);`, an
  initializer GLSL ES 3.00 introduced) becomes `const vec4 C_0 = a;`,
  `const vec4 C_1 = b;` and `vec4 C_at(int)` returning them through an `if`
  chain; `C[i]` becomes `C_at(i)`. GLSL ES 1.00 only guarantees
  constant-index-expressions when indexing in a fragment shader, which this
  doesn't rely on. Non-const arrays are left as they are.

The `legacy_psdk_*` cases are shaders of a released game (Forêt Éternelle,
PSDK 26.42), as its engine hands them to LiteCGSS.

## Behaviors worth knowing

- **Shaders compensating the NPOT padding themselves keep padded
  coordinates.** A fragment shader that uses `v_factor_npot` (beyond
  redeclaring it) was written for texture coordinates over the padded storage,
  as SFML ES gives them, and divides by the factor where it needs logical ones:
  its `gl_TexCoord[0|1]` become the raw `v_texture_coordinates` /
  `v_inv_texture_coordinates` and its `texture2D` UVs are not scaled.
  Normalizing them too divided its coordinates twice (PSDK Technical Demo's
  title button and map overlays, off-center on GL ES). New shaders should
  ignore `v_factor_npot`. (See `padded_demo_*.frag`.)

- **Comments are honored.** `gl_Color` inside `// ...` or `/* ... */` is not
  rewritten. (See `comment_awareness.frag` in the test cases.)
- **Multi-declarator** `uniform sampler2D foo, bar;` declares both samplers.
  (See `multi_declarator_sampler.frag`.)
- **Precision-qualified samplers** (`uniform highp sampler2D tex;`) are
  recognized — the discovery pass skips the qualifier. (See
  `precision_qualified_sampler.frag`.)
- **`texture2DProj` and `texture2DLod`** are wrapped the same way as
  `texture2D`.
- **Function-parameter samplers**: helpers like
  `vec4 blur(sampler2D image, vec2 uv)` get a companion `vec2 image_factor_npot`
  parameter injected automatically, and every call site gets the matching
  factor argument inserted. Chained helpers thread the factor through (see
  `chained_helpers.frag`); manually-supplied companion parameters are
  detected and not duplicated (see `factor_param_collision.frag`); forward
  declarations are rewritten to match their definitions (see
  `forward_decl_signature_injection.frag`).
- **Parenthesized sampler arguments**: `helper((texture), uv)` and
  `texture2D((s), uv)` are treated the same as the bare-identifier form.
  Balanced enclosing parens are peeled inside `bareIdentBetween`.
- **`#define` alias macros**: simple `#define MY_TEX texture` aliases are
  tracked during symbol discovery. The macro identifier is left in the
  output (the GLSL compiler's own preprocessor expands it before
  compilation), but the rewriter resolves the alias chain when forming
  factor identifiers (`texture_factor_npot`, not `MY_TEX_factor_npot`).
  Alias chains (`A → B → texture`) and parenthesized bodies
  (`#define TEX (texture)`) are handled; function-like macros and
  multi-token bodies are deliberately not tracked.

## Out of scope (deliberately)

- Full GLSL parsing / AST construction. Token-stream plus targeted analysis only.
- `#if 0 ... #endif` block awareness. Single-line `#...` directives are
  preserved as opaque `Preproc` tokens (so identifiers in them aren't
  rewritten), but multi-line conditional blocks still see their internal
  identifiers rewritten. The compiler discards inactive branches anyway, so
  the rewrite is invisible.
- Function-like / multi-token `#define` macros (`#define BLEND(a, b) ...`,
  `#define HALF vec2(0.5)`). Discovery skips these and the rewriter sees
  the literal macro name as an unknown identifier.
- `#undef`. Aliases are tracked once-and-for-all per source; scope-aware
  redefinition isn't supported.
- Function overloading. GLSL ES 1.00 doesn't support it, and GLSL ES 3.00+
  overloads with the same sampler-param shape would still get the same
  signature rewrite. Function pointers / first-class functions don't exist
  in GLSL, so call-site indirection through a variable is a non-issue.
