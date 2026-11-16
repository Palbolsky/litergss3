// Focused unit tests for the GLSL lexer. The goal is not to validate every
// possible GLSL token — it's to pin down the invariants the rewriter relies
// on: (1) round-trip fidelity (concatenating token text reproduces the input),
// (2) comments and preproc lines are NOT classified as Ident, (3) common
// patterns (multi-declarator decls, precision-qualified samplers, gl_TexCoord
// indexing) come out as expected token shapes.

#include "Backend/Shader/preprocessor/glsl_lexer.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

using cgss::glsl::lex;
using cgss::glsl::Token;
using cgss::glsl::TokenKind;
using cgss::glsl::text;

namespace {

// Reconstructs the input by concatenating each token's text in order.
std::string roundtrip(std::string_view input) {
	const auto tokens = lex(input);
	std::string out;
	for (const auto& t : tokens) {
		out.append(text(t, input));
	}
	return out;
}

// Counts tokens of a given kind.
size_t countKind(const std::vector<Token>& tokens, TokenKind k) {
	size_t n = 0;
	for (const auto& t : tokens) {
		if (t.kind == k) ++n;
	}
	return n;
}

// Returns the text of the first token matching `k`, or empty if none.
std::string_view firstOfKind(const std::vector<Token>& tokens, std::string_view source, TokenKind k) {
	for (const auto& t : tokens) {
		if (t.kind == k) return text(t, source);
	}
	return {};
}

// Indices of all Ident tokens whose text equals `name`.
std::vector<size_t> identsNamed(const std::vector<Token>& tokens, std::string_view source, std::string_view name) {
	std::vector<size_t> idx;
	for (size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].kind == TokenKind::Ident && text(tokens[i], source) == name) {
			idx.push_back(i);
		}
	}
	return idx;
}

} // namespace

TEST(GlslLexer, RoundTripsEmptyString) {
	EXPECT_EQ("", roundtrip(""));
}

TEST(GlslLexer, RoundTripsSimpleShader) {
	const std::string src =
		"uniform sampler2D texture;\n"
		"void main() {\n"
		"  gl_FragColor = texture2D(texture, gl_TexCoord[0].xy);\n"
		"}\n";
	EXPECT_EQ(src, roundtrip(src));
}

TEST(GlslLexer, RoundTripsCommentsPreprocAndOddSpacing) {
	const std::string src =
		"// hello\n"
		"/* block\n   comment */\n"
		"#ifdef GL_ES\n"
		"precision mediump float;\n"
		"#endif\n"
		"\t\tvec4   v = vec4(1.0, 2.0, 3.0, 4.0);\r\n";
	EXPECT_EQ(src, roundtrip(src));
}

TEST(GlslLexer, IdentTokensExcludeCommentBodies) {
	// "gl_Color" appears literally three times in the source: once in a line
	// comment, once in a block comment, once as real code. The lexer must
	// classify only the third as Ident.
	const std::string src =
		"// gl_Color is referenced here\n"
		"/* and gl_Color here */\n"
		"vec4 frag = gl_Color;\n";
	const auto tokens = lex(src);
	const auto ids = identsNamed(tokens, src, "gl_Color");
	EXPECT_EQ(1u, ids.size());
}

TEST(GlslLexer, IdentTokensExcludePreprocBodies) {
	// "gl_Color" in a #if 0 block still appears as part of a Preproc token
	// when the directive is single-line. (Multi-line #if blocks are not
	// special-cased by design — same scope behavior as the regex impl.)
	const std::string src = "#define MY_COLOR gl_Color\n";
	const auto tokens = lex(src);
	EXPECT_EQ(0u, identsNamed(tokens, src, "gl_Color").size());
	EXPECT_EQ(1u, countKind(tokens, TokenKind::Preproc));
}

TEST(GlslLexer, MultiDeclaratorUniformProducesIdentsForEachName) {
	// The rewriter needs to walk uniform sampler2D foo, bar; and pick up both.
	// Verify both names come out as Ident tokens.
	const std::string src = "uniform sampler2D foo, bar;\n";
	const auto tokens = lex(src);
	EXPECT_EQ(1u, identsNamed(tokens, src, "foo").size());
	EXPECT_EQ(1u, identsNamed(tokens, src, "bar").size());
}

TEST(GlslLexer, PrecisionQualifiedSamplerKeepsAllIdents) {
	const std::string src = "uniform highp sampler2D tex;\n";
	const auto tokens = lex(src);
	EXPECT_EQ(1u, identsNamed(tokens, src, "uniform").size());
	EXPECT_EQ(1u, identsNamed(tokens, src, "highp").size());
	EXPECT_EQ(1u, identsNamed(tokens, src, "sampler2D").size());
	EXPECT_EQ(1u, identsNamed(tokens, src, "tex").size());
}

TEST(GlslLexer, NumberTokensCoverFloatsAndSuffixes) {
	// Each of these is a single Number token. The lexer doesn't validate
	// against the GLSL ES version — `1.f` and `2u` may not compile under
	// GLSL ES 1.00, but the lexer's job is to tokenize, not validate.
	const std::vector<std::string> cases = {
		"1", "1.0", "1.", ".5", "1.5e-3", "1e10", "1.0f", "1.f", "2u", "0xFF"
	};
	for (const auto& src : cases) {
		const auto tokens = lex(src);
		ASSERT_FALSE(tokens.empty()) << "no tokens for: " << src;
		EXPECT_EQ(TokenKind::Number, tokens.front().kind) << "wrong kind for: " << src;
		EXPECT_EQ(src.size(), tokens.front().length) << "wrong length for: " << src;
	}
}

TEST(GlslLexer, PunctuationMultiCharOperatorsAreOneToken) {
	const std::vector<std::string> cases = {"==", "!=", "<=", ">=", "&&", "||", "++", "<<=", "..."};
	for (const auto& src : cases) {
		const auto tokens = lex(src);
		ASSERT_EQ(1u, tokens.size()) << src;
		EXPECT_EQ(TokenKind::Punct, tokens.front().kind) << src;
		EXPECT_EQ(src.size(), tokens.front().length) << src;
	}
}

TEST(GlslLexer, CrlfFoldedIntoSingleNewlineToken) {
	const std::string src = "a\r\nb";
	const auto tokens = lex(src);
	size_t nl = 0;
	for (const auto& t : tokens) {
		if (t.kind == TokenKind::Newline) {
			++nl;
			EXPECT_EQ(2u, t.length) << "CRLF should be one token of length 2";
		}
	}
	EXPECT_EQ(1u, nl);
}

TEST(GlslLexer, HashOnlyTriggersPreprocAtLineStart) {
	// "#" in the middle of a line is NOT a directive. We pass it through as
	// Other (rare malformed input) rather than swallowing the rest of the line.
	const std::string src = "foo #not_a_directive\n";
	const auto tokens = lex(src);
	EXPECT_EQ(0u, countKind(tokens, TokenKind::Preproc));
	EXPECT_EQ(roundtrip(src), src);
}
