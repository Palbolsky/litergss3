// GLSL lexer for the LiteCGSS shader preprocessor. Generated from this file via:
//   re2c -W --no-generation-date -o glsl_lexer.cpp glsl_lexer.re
// The generated file is committed; makers do not need re2c at build time.
//
// Design notes:
//   - Minimal grammar: identifiers, numbers, punctuation, comments, preproc
//     directives, whitespace, newlines, and a catch-all Other. No keyword
//     classification at the lexer level — the rewriter matches by text.
//   - "#" is only treated as a preprocessor directive when it is the first
//     non-whitespace byte on a line. This is tracked with a tiny
//     `at_line_start` bit threaded through the loop.
//   - The lexer never fails. Any unmatched byte falls through to a 1-byte
//     Other token, so malformed shaders flow through the rest of the
//     pipeline untouched.

#include "glsl_lexer.h"

#include <cstring>

namespace cgss::glsl {

namespace {

inline void push(std::vector<Token>& out, TokenKind kind, const char* start, const char* cur, const char* base) {
	Token t;
	t.kind   = kind;
	t.offset = static_cast<uint32_t>(start - base);
	t.length = static_cast<uint32_t>(cur - start);
	out.push_back(t);
}

} // namespace

std::vector<Token> lex(std::string_view source) {
	// re2c works with a null-terminated buffer (re2c:eof = 0). Copy into a
	// std::string so .data() has the trailing '\0' even if the caller passed
	// a non-terminated string_view.
	std::string buf(source);
	const char* const base    = buf.data();
	const char* YYCURSOR      = base;
	const char* YYMARKER      = nullptr;
	const char* const YYLIMIT = base + buf.size();

	std::vector<Token> tokens;
	tokens.reserve(source.size() / 4 + 16);

	// '#' counts as a preproc directive only when it's the first non-whitespace
	// char on a line. Track that with a bit that resets after Newline and stays
	// true through Whitespace (but flips false on anything else).
	bool at_line_start = true;

	for (;;) {
		const char* anchor = YYCURSOR;

		/*!re2c
			re2c:define:YYCTYPE    = char;
			re2c:yyfill:enable     = 0;
			re2c:eof               = 0;

			id_start  = [a-zA-Z_];
			id_cont   = [a-zA-Z0-9_];
			digit     = [0-9];
			hex       = [0-9a-fA-F];
			exp       = [eE] [+-]? digit+;
			fsuf      = [fF];
			usuf      = [uU];

			// EOF: re2c hits the sentinel '\0' and triggers $.
			$ { break; }

			// Newline. Fold CRLF into one Newline token so emit reproduces line endings.
			"\r\n"          { push(tokens, TokenKind::Newline, anchor, YYCURSOR, base); at_line_start = true; continue; }
			[\r\n]          { push(tokens, TokenKind::Newline, anchor, YYCURSOR, base); at_line_start = true; continue; }

			// Whitespace (spaces/tabs only).
			[ \t]+          { push(tokens, TokenKind::Whitespace, anchor, YYCURSOR, base); /* at_line_start unchanged */ continue; }

			// Line comment to end-of-line (not including the newline itself).
			"//" [^\r\n\x00]* { push(tokens, TokenKind::LineComment, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Block comment "/* ... */". The repetition explicitly handles a
			// bare '*' that isn't followed by '/', so the rule stops cleanly
			// at the closing "*/". An unterminated "/*" (no closing) simply
			// won't match this rule; the catch-all '*' rule then consumes a
			// single byte and the lexer continues — malformed input is the
			// shader compiler's problem to surface, not ours.
			"/*" ( [^*\x00] | ("*" [^/\x00]) )* "*""/"
			                { push(tokens, TokenKind::BlockComment, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Preprocessor directive: only when '#' is the first non-whitespace
			// byte on a line. The lexer matches '#' aggressively; we downgrade
			// to Punct + Ident chain in the (rare) non-line-start case below.
			"#" [^\r\n\x00]* {
				if (at_line_start) {
					push(tokens, TokenKind::Preproc, anchor, YYCURSOR, base);
				} else {
					// '#' mid-line is not valid GLSL but pass it through as Other
					// so we don't accidentally swallow a whole line.
					YYCURSOR = anchor + 1;
					push(tokens, TokenKind::Other, anchor, YYCURSOR, base);
				}
				at_line_start = false;
				continue;
			}

			// Identifiers / keywords.
			id_start id_cont* { push(tokens, TokenKind::Ident, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Numbers. Order matters: try floats (with decimal or exponent or suffix) before plain ints.
			// 1.0, 1., .5, 1.5e-3, 1e10, 1.0f
			digit+ "." digit* exp? fsuf? { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }
			"." digit+ exp? fsuf?        { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }
			digit+ exp fsuf?             { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }
			digit+ fsuf                  { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }
			"0" [xX] hex+ usuf?          { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }
			digit+ usuf?                 { push(tokens, TokenKind::Number, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Multi-char punctuation. Listed longest-first so re2c's longest-match
			// rule picks them up before the single-char fallbacks.
			"<<=" | ">>=" | "..."                                { push(tokens, TokenKind::Punct, anchor, YYCURSOR, base); at_line_start = false; continue; }
			"==" | "!=" | "<=" | ">=" | "&&" | "||" | "<<" | ">>"
			| "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
			| "++" | "--"                                       { push(tokens, TokenKind::Punct, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Any single non-letter/non-digit byte that isn't already handled.
			[!-/:-@[-`{-~] { push(tokens, TokenKind::Punct, anchor, YYCURSOR, base); at_line_start = false; continue; }

			// Catch-all: consume one byte as Other so we never get stuck.
			* { push(tokens, TokenKind::Other, anchor, YYCURSOR, base); at_line_start = false; continue; }
		*/
	}

	return tokens;
}

} // namespace cgss::glsl
