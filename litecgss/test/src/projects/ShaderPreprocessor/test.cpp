// Snapshot tests for cgss::ShaderPreprocessor.
//
// Each test case is a pair of files under cases/:
//   <name>.<stage>.in        — the raw GLSL source fed to the preprocessor.
//   <name>.<stage>.expected  — the exact text we expect process() to produce.
//
// <stage> is "frag" (Stage::Fragment), "vert" (Stage::Vertex) or "geom"
// (Stage::Geometry). The runner derives the stage from the filename so we
// don't have to hard-code a mapping in C++.
//
// When iterating on the preprocessor (or adding new cases), set the env var
// SHADER_PREPROCESSOR_REGENERATE_EXPECTED=1 once to write the *actual* output
// into the .expected file instead of asserting. Inspect the diff, commit if
// happy, then run the suite normally. This mirrors the "golden file" workflow
// used by clang-format / rustfmt / prettier tests.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Backend/Shader/ShaderPreprocessor.h"

namespace {

namespace fs = std::filesystem;

// LITE_CGSS_TEST_DIR is injected by the parent CMakeLists.txt and points at
// litecgss/test. Our case files live next to this test source.
const fs::path kCasesDir =
	fs::path(LITE_CGSS_TEST_DIR) / "src" / "projects" / "ShaderPreprocessor" / "cases";

std::string readFile(const fs::path& path) {
	std::ifstream in(path);
	if (!in) {
		ADD_FAILURE() << "Failed to read " << path;
		return {};
	}
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

void writeFile(const fs::path& path, const std::string& contents) {
	std::ofstream out(path, std::ios::trunc);
	out << contents;
}

bool regenerateMode() {
	const char* env = std::getenv("SHADER_PREPROCESSOR_REGENERATE_EXPECTED");
	return env != nullptr && env[0] != '\0' && env[0] != '0';
}

cgss::ShaderPreprocessor::Stage stageFromExtension(const std::string& stageTag) {
	if (stageTag == "vert") return cgss::ShaderPreprocessor::Stage::Vertex;
	if (stageTag == "geom") return cgss::ShaderPreprocessor::Stage::Geometry;
	return cgss::ShaderPreprocessor::Stage::Fragment;
}

// Parse "<caseName>.<stage>.in" into ("<caseName>.<stage>", "<stage>").
struct InputDescriptor {
	fs::path inputPath;
	fs::path expectedPath;
	std::string caseName;
	std::string stageTag;
};

std::vector<InputDescriptor> discoverCases() {
	std::vector<InputDescriptor> cases;
	if (!fs::exists(kCasesDir)) {
		ADD_FAILURE() << "Cases directory missing: " << kCasesDir;
		return cases;
	}
	for (const auto& entry : fs::directory_iterator(kCasesDir)) {
		if (!entry.is_regular_file()) continue;
		const fs::path& p = entry.path();
		if (p.extension() != ".in") continue;

		// Stem is "<caseName>.<stage>", e.g. "bare_texture2d.frag"
		const std::string stem = p.stem().string();
		const auto dot = stem.find_last_of('.');
		if (dot == std::string::npos) {
			ADD_FAILURE() << "Case file has no stage tag: " << p;
			continue;
		}

		InputDescriptor d;
		d.inputPath = p;
		d.expectedPath = p.parent_path() / (stem + ".expected");
		d.caseName = stem.substr(0, dot);
		d.stageTag = stem.substr(dot + 1);
		cases.push_back(std::move(d));
	}
	// Stable order so failures map to consistent line numbers.
	std::sort(cases.begin(), cases.end(),
	          [](const InputDescriptor& a, const InputDescriptor& b) {
		          return a.inputPath < b.inputPath;
	          });
	return cases;
}

class ShaderPreprocessorCases : public ::testing::TestWithParam<InputDescriptor> {};

TEST_P(ShaderPreprocessorCases, MatchesExpected) {
	const auto& d = GetParam();
	const std::string input = readFile(d.inputPath);
	const auto stage = stageFromExtension(d.stageTag);
	const std::string actual = cgss::ShaderPreprocessor::process(input, stage);

	if (regenerateMode()) {
		writeFile(d.expectedPath, actual);
		GTEST_SKIP() << "Regenerated expected: " << d.expectedPath.filename();
		return;
	}

	if (!fs::exists(d.expectedPath)) {
		ADD_FAILURE() << "Expected file missing: " << d.expectedPath
		              << "\nRun with SHADER_PREPROCESSOR_REGENERATE_EXPECTED=1 to create it,"
		              << " then review the contents before committing.";
		return;
	}

	const std::string expected = readFile(d.expectedPath);
	EXPECT_EQ(expected, actual)
		<< "Mismatch in case '" << d.caseName << "' (" << d.stageTag << ")."
		<< "\nIf the new output is intentional, set"
		<< " SHADER_PREPROCESSOR_REGENERATE_EXPECTED=1 and rerun to refresh"
		<< " " << d.expectedPath.filename() << ".";
}

// Generates a stable, gtest-friendly name from each descriptor.
std::string testName(const ::testing::TestParamInfo<InputDescriptor>& info) {
	std::string name = info.param.caseName + "_" + info.param.stageTag;
	for (char& c : name) {
		if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
	}
	return name;
}

INSTANTIATE_TEST_SUITE_P(All, ShaderPreprocessorCases,
                         ::testing::ValuesIn(discoverCases()),
                         testName);

// Idempotency: running the processed output through process() a second time
// must produce the same string. This guards against accidental double-injection
// of varyings / per-sampler factor declarations and double-wrapping of
// texture2D calls. Uses every discovered case as fuel so we don't have to
// curate a dedicated input set.
TEST(ShaderPreprocessorIdempotency, ProcessTwiceMatchesProcessOnce) {
	const auto cases = discoverCases();
	ASSERT_FALSE(cases.empty()) << "No cases discovered under " << kCasesDir;

	for (const auto& d : cases) {
		const std::string input = readFile(d.inputPath);
		const auto stage = stageFromExtension(d.stageTag);
		const std::string once  = cgss::ShaderPreprocessor::process(input, stage);
		const std::string twice = cgss::ShaderPreprocessor::process(once,  stage);
		EXPECT_EQ(once, twice) << "process() is not idempotent for case '"
		                       << d.caseName << "' (" << d.stageTag << ").";
	}
}

// --- GL ES adaptation of desktop shaders (the test build is the GL ES one:
// no LITECGSS_USE_FIXED_PIPELINE) ---------------------------------------------

using UniformDefault = cgss::ShaderPreprocessor::UniformDefault;

const UniformDefault* findDefault(const std::vector<UniformDefault>& defaults, const std::string& name) {
	for (const auto& d : defaults) {
		if (d.name == name) return &d;
	}
	return nullptr;
}

// GLSL ES forbids uniform initializers: they are removed from the source,
// their values reported so the engine sets them once the shader is linked.
TEST(ShaderPreprocessorGlEs, ReportsTheUniformInitializersOfAnOldPsdkShader) {
	const std::string input = readFile(kCasesDir / "legacy_psdk_overlay_water.frag.in");
	const auto result = cgss::ShaderPreprocessor::processDetailed(input, cgss::ShaderPreprocessor::Stage::Fragment);

	const struct { const char* name; const char* type; std::vector<float> values; } expected[] = {
		{ "blend_mode",  "int",   { 0.f } },
		{ "color",       "vec4",  { 0.f, 0.f, 0.f, 0.f } },
		{ "tone",        "vec4",  { 0.f, 0.f, 0.f, 0.f } },
		{ "direction1",  "vec2",  { 0.1f, 0.1f } },
		{ "direction2",  "vec2",  { -0.1f, -0.1f } },
		{ "opacity",     "float", { 1.f } },
		{ "dist_factor", "float", { 1.5f } },
	};
	EXPECT_EQ(std::size(expected), result.uniformDefaults.size());
	for (const auto& e : expected) {
		const UniformDefault* d = findDefault(result.uniformDefaults, e.name);
		ASSERT_NE(nullptr, d) << e.name;
		EXPECT_EQ(e.type, d->type) << e.name;
		EXPECT_EQ(e.values, d->values) << e.name;
	}
	// `uniform float time;` has no initializer
	EXPECT_EQ(nullptr, findDefault(result.uniformDefaults, "time"));
	EXPECT_EQ(std::string::npos, result.source.find("dist_factor ="));
}

TEST(ShaderPreprocessorGlEs, ReportsBoolAndSplatInitializers) {
	const auto result = cgss::ShaderPreprocessor::processDetailed(
		"uniform bool in_snapshot = false;\n"
		"uniform highp vec4 tint = vec4(1.0), shade = vec4(0.5, 0.5, 0.5, 1.0);\n"
		"uniform float unknown = SOME_MACRO;\n"
		"void main() { gl_FragColor = tint * shade; }\n",
		cgss::ShaderPreprocessor::Stage::Fragment);

	ASSERT_EQ(3u, result.uniformDefaults.size());
	EXPECT_EQ((std::vector<float>{ 0.f }), findDefault(result.uniformDefaults, "in_snapshot")->values);
	EXPECT_EQ((std::vector<float>{ 1.f, 1.f, 1.f, 1.f }), findDefault(result.uniformDefaults, "tint")->values);
	EXPECT_EQ((std::vector<float>{ 0.5f, 0.5f, 0.5f, 1.f }), findDefault(result.uniformDefaults, "shade")->values);
	// Not a constant: removed (GLSL ES refuses it) but not reported
	EXPECT_EQ(nullptr, findDefault(result.uniformDefaults, "unknown"));
	EXPECT_NE(std::string::npos, result.source.find("uniform highp vec4 tint, shade;"));
	EXPECT_NE(std::string::npos, result.source.find("uniform float unknown;"));
}

// A fragment-only shader is paired with this one on GL ES: it is the desktop
// fixed-pipeline vertex shader, rewritten.
TEST(ShaderPreprocessorGlEs, DefaultVertexShaderWritesTheFragmentVaryings) {
	const std::string& vertex = cgss::ShaderPreprocessor::defaultVertexShader();
	for (const char* fixedPipeline : { "gl_Vertex", "gl_MultiTexCoord0", "gl_TextureMatrix", "gl_ModelViewProjectionMatrix",
	                                   "gl_FrontColor", "gl_Color", "gl_TexCoord" }) {
		EXPECT_EQ(std::string::npos, vertex.find(fixedPipeline)) << fixedPipeline;
	}
	for (const char* line : { "attribute vec2 position;", "varying vec2 v_texture_coordinates;",
	                          "gl_Position = sf_ModelViewProjectionMatrix * sf_Vertex;",
	                          "v_texture_coordinates = sf_TexCoord[0].xy * factor_npot;", "v_factor_npot = factor_npot;" }) {
		EXPECT_NE(std::string::npos, vertex.find(line)) << line;
	}
}

TEST(ShaderPreprocessorGlEs, OnlyDesktopVersionsAreReplaced) {
	using cgss::ShaderPreprocessor;
	const auto version = [](const std::string& source) {
		const std::string out = ShaderPreprocessor::process(source, ShaderPreprocessor::Stage::Vertex);
		return out.substr(0, out.find('\n'));
	};
	EXPECT_EQ("#version 100", version("#version 130\nvoid main() {}\n"));
	EXPECT_EQ("#version 100", version("#version 120 compatibility\nvoid main() {}\n"));
	EXPECT_EQ("#version 100", version("#  version 110\r\nvoid main() {}\r\n"));
	EXPECT_EQ("#version 100", version("#version 100\nvoid main() {}\n"));
	EXPECT_EQ("#version 300 es", version("#version 300 es\nvoid main() {}\n"));
	// Comments before the directive stay in place
	EXPECT_EQ("// header", version("// header\n#version 130\nvoid main() {}\n"));
	EXPECT_NE(std::string::npos,
	          ShaderPreprocessor::process("// header\n#version 130\nvoid main() {}\n", ShaderPreprocessor::Stage::Vertex)
	              .find("// header\n#version 100\n"));
}

// Integer literals become float ones only next to an operand known to be a
// float: int arithmetic, indices and constructor arguments stay as written.
TEST(ShaderPreprocessorGlEs, PromotesIntegerLiteralsOnlyNextToFloats) {
	const std::string out = cgss::ShaderPreprocessor::process(
		"uniform sampler2D texture;\n"
		"uniform int count;\n"
		"uniform float time;\n"
		"vec2 offset(vec2 uv, int i) { return uv + vec2(i, 0) * 2; }\n"
		"void main() {\n"
		"  float half_time = time / 2;\n"
		"  float t = 1;\n"
		"  vec4 frag = texture2D(texture, offset(gl_TexCoord[0].xy, count - 1));\n"
		"  for (int i = 0; i < 4; i++) { frag.a -= 0.1 * float(i + 1); }\n"
		"  if (count > 2) frag.rgb = 1 - frag.rgb;\n"
		"  gl_FragColor = frag * (1 + half_time) * -1 + 2 * texture2D(texture, vec2(0.5)).a;\n"
		"}\n",
		cgss::ShaderPreprocessor::Stage::Fragment);

	for (const char* promoted : { "vec2(i, 0) * 2.0", "time / 2.0", "float t = 1.0;", "frag.rgb = 1.0 - frag.rgb",
	                              "(1.0 + half_time) * -1.0", "+ 2.0 * texture2D" }) {
		EXPECT_NE(std::string::npos, out.find(promoted)) << promoted << "\n" << out;
	}
	for (const char* kept : { "count - 1)", "int i = 0; i < 4;", "float(i + 1)", "count > 2" }) {
		EXPECT_NE(std::string::npos, out.find(kept)) << kept << "\n" << out;
	}
}

// A const array indexed with a computed index (PSDK's title_button.frag):
// GLSL ES 1.00 fragment shaders are only guaranteed constant indices, so
// the array becomes constants and an accessor.
TEST(ShaderPreprocessorGlEs, ConstArraysBecomeConstantsAndAnAccessor) {
	const std::string out = cgss::ShaderPreprocessor::process(
		"const float WEIGHTS[3] = float[](1, 0.5, 0.25);\n"
		"vec4 tints[2];\n"
		"uniform float t;\n"
		"void main() {\n"
		"  float w = WEIGHTS[int(floor(t * 3.0))] + WEIGHTS[0];\n"
		"  tints[0] = vec4(w);\n"
		"  gl_FragColor = tints[0];\n"
		"}\n",
		cgss::ShaderPreprocessor::Stage::Fragment);

	for (const char* expected : { "const float WEIGHTS_0 = 1.0;\n", "const float WEIGHTS_1 = 0.5;\n",
	                              "const float WEIGHTS_2 = 0.25;\n",
	                              "float WEIGHTS_at(int sf_index) {\n"
	                              "    if (sf_index == 0) return WEIGHTS_0;\n"
	                              "    if (sf_index == 1) return WEIGHTS_1;\n"
	                              "    return WEIGHTS_2;\n}",
	                              "float w = WEIGHTS_at(int(floor(t * 3.0))) + WEIGHTS_at(0);",
	                              // Not const: may be written, left as is
	                              "vec4 tints[2];", "tints[0] = vec4(w);" }) {
		EXPECT_NE(std::string::npos, out.find(expected)) << expected << "\n" << out;
	}
	EXPECT_EQ(std::string::npos, out.find("WEIGHTS["));
}

} // namespace
