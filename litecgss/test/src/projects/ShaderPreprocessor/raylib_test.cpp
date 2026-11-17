// raylib targets: every case input of cases/ goes through Target::RaylibGlsl330
// (golden files under cases/raylib/) and Target::RaylibGlslEs300 (under
// cases/raylib_es300/), regenerated like the main suite with
// SHADER_PREPROCESSOR_REGENERATE_EXPECTED=1. Outputs must be GLSL 330 core /
// GLSL ES 3.00: no fixed-pipeline built-in, no GLSL 1.x syntax. The ES 3.00
// golden files are also compiled by glslangValidator when it is installed
// (CTest LiteCGSS.ShaderPreprocessor.glslang).

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "Backend/Shader/ShaderPreprocessor.h"

namespace {

namespace fs = std::filesystem;
using Preprocessor = cgss::ShaderPreprocessor;

const fs::path kCasesDir = fs::path(LITE_CGSS_TEST_DIR) / "src" / "projects" / "ShaderPreprocessor" / "cases";

std::string readAll(const fs::path& path) {
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

bool regenerate() {
	const char* env = std::getenv("SHADER_PREPROCESSOR_REGENERATE_EXPECTED");
	return env != nullptr && env[0] != '\0' && env[0] != '0';
}

struct Case { fs::path input; Preprocessor::Stage stage; };

std::vector<Case> raylibCases() {
	std::vector<Case> cases;
	for (const auto& entry : fs::directory_iterator(kCasesDir)) {
		const std::string name = entry.path().filename().string();
		if (name.size() < 3 || name.substr(name.size() - 3) != ".in") continue;
		if (name.find(".frag.") != std::string::npos) cases.push_back({ entry.path(), Preprocessor::Stage::Fragment });
		else if (name.find(".vert.") != std::string::npos) cases.push_back({ entry.path(), Preprocessor::Stage::Vertex });
	}
	std::sort(cases.begin(), cases.end(), [](const Case& a, const Case& b) { return a.input < b.input; });
	return cases;
}

// Strips comments, so the checks below only look at code.
std::string codeOnly(const std::string& source) {
	static const std::regex comments(R"(//[^\n]*|/\*[\s\S]*?\*/)");
	return std::regex_replace(source, comments, "");
}

struct RaylibTarget {
	const char* name;
	Preprocessor::Target target;
	const char* goldenDir;
	const char* versionLine;
};

const RaylibTarget kTargets[] = {
	{ "Glsl330",   Preprocessor::Target::RaylibGlsl330,   "raylib",       "#version 330\n" },
	{ "GlslEs300", Preprocessor::Target::RaylibGlslEs300, "raylib_es300", "#version 300 es\n" },
};

class ShaderPreprocessorRaylib : public ::testing::TestWithParam<RaylibTarget> {
protected:
	std::string process(const std::string& source, Preprocessor::Stage stage) const {
		return Preprocessor::process(source, stage, Preprocessor::Options{ GetParam().target });
	}
};

} // namespace

TEST_P(ShaderPreprocessorRaylib, MatchesGoldenFiles) {
	const auto cases = raylibCases();
	ASSERT_FALSE(cases.empty());
	const fs::path goldenDir = kCasesDir / GetParam().goldenDir;
	if (regenerate()) fs::create_directories(goldenDir);
	for (const auto& c : cases) {
		const std::string actual = process(readAll(c.input), c.stage);
		const fs::path expectedPath = goldenDir / (c.input.stem().string() + ".expected");
		if (regenerate()) {
			std::ofstream(expectedPath, std::ios::trunc) << actual;
			continue;
		}
		ASSERT_TRUE(fs::exists(expectedPath)) << "missing " << expectedPath;
		EXPECT_EQ(readAll(expectedPath), actual) << c.input.filename();
	}
	// The default stages too: the partner of each golden file when it is
	// compiled for real (glslang, the ShaderRuntime test).
	const std::pair<const char*, std::string> defaults[] = {
		{ "default.vert.expected", Preprocessor::defaultVertexShader(GetParam().target) },
		{ "default.frag.expected", Preprocessor::defaultFragmentShader(GetParam().target) },
	};
	for (const auto& [name, actual] : defaults) {
		if (regenerate()) {
			std::ofstream(goldenDir / name, std::ios::trunc) << actual;
			continue;
		}
		ASSERT_TRUE(fs::exists(goldenDir / name)) << "missing " << goldenDir / name;
		EXPECT_EQ(readAll(goldenDir / name), actual) << name;
	}
}

TEST_P(ShaderPreprocessorRaylib, OutputHasNoLegacySyntax) {
	static const std::regex legacy(
		R"(\b(gl_TexCoord|gl_Color|gl_FrontColor|gl_FragColor|gl_Vertex|gl_MultiTexCoord0|gl_ModelViewProjectionMatrix|gl_TextureMatrix|attribute|varying|texture2D|texture2DLod|texture2DProj)\b)");
	static const std::regex sampler(R"(\bsampler2D\s+texture\b)");
	for (const auto& c : raylibCases()) {
		const std::string out = process(readAll(c.input), c.stage);
		EXPECT_EQ(0u, out.rfind(GetParam().versionLine, 0)) << c.input.filename();
		const std::string code = codeOnly(out);
		std::smatch m;
		EXPECT_FALSE(std::regex_search(code, m, legacy)) << c.input.filename() << ": " << m.str();
		EXPECT_FALSE(std::regex_search(code, sampler)) << c.input.filename();
	}
}

TEST_P(ShaderPreprocessorRaylib, IsIdempotent) {
	for (const auto& c : raylibCases()) {
		const std::string once = process(readAll(c.input), c.stage);
		EXPECT_EQ(once, process(once, c.stage)) << c.input.filename();
	}
}

TEST_P(ShaderPreprocessorRaylib, DefaultStagesAreOnRaylibsAttributes) {
	const auto vs = Preprocessor::defaultVertexShader(GetParam().target);
	const auto fs = Preprocessor::defaultFragmentShader(GetParam().target);
	EXPECT_EQ(0u, vs.rfind(GetParam().versionLine, 0));
	EXPECT_EQ(0u, fs.rfind(GetParam().versionLine, 0));
	EXPECT_NE(std::string::npos, vs.find("in vec3 vertexPosition;"));
	EXPECT_NE(std::string::npos, vs.find("out vec4 sf_color;"));
	EXPECT_NE(std::string::npos, fs.find("in vec4 sf_color;"));
	EXPECT_NE(std::string::npos, fs.find("uniform sampler2D texture0;"));
}

TEST_P(ShaderPreprocessorRaylib, RenamesTheTextureSampler) {
	EXPECT_EQ("texture0", Preprocessor::uniformName("texture", GetParam().target));
	EXPECT_EQ("noise", Preprocessor::uniformName("noise", GetParam().target));
	EXPECT_EQ("texture", Preprocessor::uniformName("texture", Preprocessor::Target::SfmlEs100));
}

INSTANTIATE_TEST_SUITE_P(Targets, ShaderPreprocessorRaylib, ::testing::ValuesIn(kTargets),
	[](const ::testing::TestParamInfo<RaylibTarget>& info) { return std::string(info.param.name); });

// GLSL ES 3.00 forbids implicit int -> float conversions and uniform
// initializers (GLSL 330 allows both): those are rewritten, the GLSL ES 1.00
// workarounds (%, round(), const arrays) are not, 3.00 has them.
TEST(ShaderPreprocessorRaylibEs300, PromotesIntegerLiteralsNextToFloats) {
	const std::string out = Preprocessor::process(
		"uniform sampler2D texture;\n"
		"void main() {\n"
		"    vec2 uv = gl_TexCoord[0].xy;\n"
		"    float y = 1 - uv.y;\n"
		"    gl_FragColor = texture2D(texture, vec2(uv.x, y)) * 2;\n"
		"}\n",
		Preprocessor::Stage::Fragment, Preprocessor::Options{ Preprocessor::Target::RaylibGlslEs300 });
	EXPECT_NE(std::string::npos, out.find("1.0 - uv.y")) << out;
	EXPECT_NE(std::string::npos, out.find("* 2.0")) << out;
}

TEST(ShaderPreprocessorRaylibEs300, ReportsUniformInitializers) {
	const auto result = Preprocessor::processDetailed(
		"uniform float opacity = 0.5;\n"
		"uniform sampler2D texture;\n"
		"void main() { gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * opacity; }\n",
		Preprocessor::Stage::Fragment, Preprocessor::Options{ Preprocessor::Target::RaylibGlslEs300 });
	ASSERT_EQ(1u, result.uniformDefaults.size());
	EXPECT_EQ("opacity", result.uniformDefaults[0].name);
	EXPECT_EQ(std::string::npos, result.source.find("= 0.5")) << result.source;
}

TEST(ShaderPreprocessorRaylibEs300, KeepsWhatGlslEs300Has) {
	const std::string out = Preprocessor::process(
		"uniform sampler2D texture;\n"
		"const float WEIGHTS[2] = float[2](1.0, 0.5);\n"
		"void main() {\n"
		"    int i = 7 % 2;\n"
		"    gl_FragColor = texture2D(texture, gl_TexCoord[0].xy) * round(WEIGHTS[i]);\n"
		"}\n",
		Preprocessor::Stage::Fragment, Preprocessor::Options{ Preprocessor::Target::RaylibGlslEs300 });
	EXPECT_NE(std::string::npos, out.find("7 % 2")) << out;
	EXPECT_NE(std::string::npos, out.find("const float WEIGHTS[2]")) << out;
	EXPECT_EQ(std::string::npos, out.find("cgss_round")) << out;
}
