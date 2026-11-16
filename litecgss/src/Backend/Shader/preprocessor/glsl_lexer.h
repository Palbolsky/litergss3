#pragma once

// Minimal GLSL tokenizer used by the LiteCGSS shader preprocessor. The grammar
// recognized here is intentionally tiny — just enough to drive the rewriter
// (comment-aware, scope-aware, sampler-aware). It does NOT validate GLSL: any
// byte sequence is consumed and emitted back as some token kind, so malformed
// shaders pass through untouched rather than failing the preprocessor.
//
// Each token carries its source offset and length; emission reproduces the
// original bytes via string_view slicing, which preserves whitespace and
// comments exactly. Rewrites are applied by substituting specific token spans
// at emit time.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cgss::glsl {

enum class TokenKind : uint8_t {
	// Identifier or keyword. Keyword discrimination happens via text match
	// in the symbol/rewrite passes — keeping the lexer small.
	Ident,
	// Numeric literal: integer or float, with optional exponent/suffix.
	Number,
	// Single-char or multi-char punctuation/operator. Each Punct token is one
	// lexeme (e.g. "==", "<<=", "++") — never split across tokens.
	Punct,
	// "// ..." up to but NOT including the terminating newline.
	LineComment,
	// "/* ... */", including the delimiters.
	BlockComment,
	// "#..." preprocessor directive on its own line, up to but NOT including
	// the terminating newline. Only recognized when '#' is the first
	// non-whitespace char on a line.
	Preproc,
	// Run of spaces / tabs (no newlines).
	Whitespace,
	// One newline. "\r\n" is folded into a single token of length 2 so
	// emission keeps the original line-ending style.
	Newline,
	// Anything else (stray byte). Emitted verbatim. Should be rare.
	Other,
	// Sentinel; not appended to the token list, but useful as a return.
	EndOfFile,
};

struct Token {
	TokenKind kind;
	uint32_t  offset;  // byte index into the source passed to lex()
	uint32_t  length;
};

// Tokenizes `source` into a vector of Tokens. The lexer never fails: any
// unrecognized byte becomes a single Other token. The returned vector covers
// every byte of `source` exactly once when iterated in order (so concatenating
// text(t, source) for every token reproduces the input byte-for-byte).
std::vector<Token> lex(std::string_view source);

// Convenience: extract the byte range backing this token.
inline std::string_view text(const Token& t, std::string_view source) {
	return source.substr(t.offset, t.length);
}

// True iff this token is "transparent" to the rewriter — it never represents
// semantically meaningful GLSL identifiers. Used by the rewriter to skip past
// whitespace/comments when looking for argument boundaries.
inline bool is_trivia(TokenKind k) {
	return k == TokenKind::Whitespace
	    || k == TokenKind::Newline
	    || k == TokenKind::LineComment
	    || k == TokenKind::BlockComment;
}

// --- Shared token-stream helpers -------------------------------------------
//
// Used by both the symbol-discovery pass and the rewrite pass. Kept inline in
// this header so neither consumer needs to import the other's translation
// unit; the functions are small enough that link-time deduplication is fine.

// True if `t` is a single-character punctuation token whose byte equals `c`.
// Punct tokens are always made from one or more bytes of ASCII punctuation,
// so length-1 + byte-match is the cheapest unambiguous test.
inline bool isPunctChar(const Token& t, std::string_view source, char c) {
	return t.kind == TokenKind::Punct
	    && t.length == 1
	    && source[t.offset] == c;
}

// Advances past any trivia (whitespace, newlines, comments) starting at index
// `i`. Returns the index of the first non-trivia token at or after `i`, or
// `tokens.size()` if the rest of the stream is all trivia.
inline std::size_t skipTrivia(const std::vector<Token>& tokens, std::size_t i) {
	while (i < tokens.size() && is_trivia(tokens[i].kind)) {
		++i;
	}
	return i;
}

// Value-semantic position in a token stream. Each navigation method returns a
// new cursor — the original is unchanged — so snapshot/backtrack idioms work
// naturally:
//
//     auto save = cur;
//     cur = cur.next();
//     if (cur.is(TokenKind::Punct)) { cur = save; }   // bail out, restore
//
// All accessors (`kind()`, `text()`, `token()`) return defaults at eof so
// callers don't have to guard every dereference. Predicates (`is`, `isPunct`,
// `isText`, `operator==`) also short-circuit to false at eof.
class TokenCursor {
public:
	TokenCursor() = default;
	TokenCursor(const std::vector<Token>& tokens, std::string_view source, std::size_t idx = 0)
		: tokens_(&tokens), source_(source), idx_(idx) {}

	bool        eof()   const { return !tokens_ || idx_ >= tokens_->size(); }
	std::size_t index() const { return idx_; }
	TokenKind   kind()  const { return eof() ? TokenKind::EndOfFile : (*tokens_)[idx_].kind; }
	std::string_view text() const { return eof() ? std::string_view() : cgss::glsl::text((*tokens_)[idx_], source_); }
	const Token& token() const { return (*tokens_)[idx_]; } // UB if eof — callers must guard
	const std::vector<Token>& stream() const { return *tokens_; }
	std::string_view source() const { return source_; }

	// A cursor parked past the last token of the same stream. Useful as the
	// "end" sentinel when calling range-based helpers like bareIdentBetween.
	TokenCursor end() const {
		return tokens_ ? TokenCursor(*tokens_, source_, tokens_->size()) : TokenCursor();
	}

	// Navigation. Out-of-range moves clamp to the past-the-end position
	// rather than wrapping or asserting; callers can check `eof()` after.
	TokenCursor next(std::size_t n = 1) const {
		if (!tokens_) return *this;
		const std::size_t limit = tokens_->size();
		return TokenCursor(*tokens_, source_, idx_ + n > limit ? limit : idx_ + n);
	}
	TokenCursor prev(std::size_t n = 1) const {
		return TokenCursor(*tokens_, source_, n > idx_ ? 0 : idx_ - n);
	}
	TokenCursor skipTrivia() const {
		if (!tokens_) return *this;
		std::size_t i = idx_;
		const std::size_t limit = tokens_->size();
		while (i < limit && is_trivia((*tokens_)[i].kind)) ++i;
		return TokenCursor(*tokens_, source_, i);
	}
	// Skips trivia and any Idents whose text matches one of the supplied
	// names. Returns the cursor parked on the first non-matching token (or
	// eof). Used to pass over storage / precision qualifiers like
	// `highp sampler2D` or `in vec2`.
	template <class... Names>
	TokenCursor skipIdents(Names&&... names) const {
		auto c = skipTrivia();
		while (!c.eof() && c.kind() == TokenKind::Ident && ((c.text() == names) || ...)) {
			c = c.next().skipTrivia();
		}
		return c;
	}

	// Predicates. All short-circuit to false at eof.
	bool is(TokenKind k)          const { return !eof() && kind() == k; }
	bool isPunct(char c)          const { return !eof() && isPunctChar((*tokens_)[idx_], source_, c); }
	bool isIdent()                const { return is(TokenKind::Ident); }
	bool isText(std::string_view s) const { return !eof() && text() == s; }
	bool isTrivia()               const { return !eof() && is_trivia(kind()); }

	// Sugared forms requested by callers that prefer the operator style.
	TokenCursor operator+(std::size_t n) const { return next(n); }
	bool operator==(TokenKind k)            const { return is(k); }
	bool operator!=(TokenKind k)            const { return !is(k); }
	bool operator==(std::string_view s)     const { return isText(s); }
	bool operator!=(std::string_view s)     const { return !isText(s); }

	// Position comparison between cursors. Only meaningful for cursors over
	// the same stream — that's the only legal usage.
	bool operator<(const TokenCursor& other)  const { return idx_ < other.idx_; }
	bool operator==(const TokenCursor& other) const { return idx_ == other.idx_; }
	bool operator!=(const TokenCursor& other) const { return idx_ != other.idx_; }

private:
	const std::vector<Token>* tokens_ = nullptr;
	std::string_view          source_;
	std::size_t               idx_ = 0;
};

// Returns the text of the identifier in [begin, end) when that range contains
// nothing but the identifier and (optionally) balanced enclosing parentheses
// around it — i.e. accepts `foo`, `(foo)`, `((foo))`; rejects `foo + bar`,
// `f(x)`, etc. Used by parsers that need "is this a bare reference to one
// thing?" decisions (the macro alias body, function-call sampler arguments,
// etc.).
inline std::string_view bareIdentBetween(TokenCursor begin, TokenCursor end) {
	auto        c          = begin.skipTrivia();
	std::size_t parenDepth = 0;
	while (c < end && c.isPunct('(')) {
		++parenDepth;
		c = c.next().skipTrivia();
	}
	if (!(c < end) || !c.isIdent()) return {};
	const auto ident = c.text();
	c = c.next().skipTrivia();
	while (parenDepth > 0 && c < end && c.isPunct(')')) {
		--parenDepth;
		c = c.next().skipTrivia();
	}
	if (c != end || parenDepth != 0) return {};
	return ident;
}

// Single-argument overload: "from begin to end of begin's stream". Saves the
// caller from spelling out the end sentinel when consuming everything left
// in the stream (e.g. the body of a `#define` directive).
inline std::string_view bareIdentBetween(TokenCursor begin) {
	return bareIdentBetween(begin, begin.end());
}

} // namespace cgss::glsl
