// Target::RaylibGlsl330: every case input of cases/ goes through the raylib
// target; outputs are golden files under cases/raylib/ (regenerated like the
// main suite, with SHADER_PREPROCESSOR_REGENERATE_EXPECTED=1), and must be
// GLSL 330 core: no fixed-pipeline built-in, no GLSL 1.x syntax.

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

const fs::path kCasesDir  = fs::path(LITE_CGSS_TEST_DIR) / "src" / "projects" / "ShaderPreprocessor" / "cases";
const fs::path kRaylibDir = kCasesDir / "raylib";

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

std::string processRaylib(const std::string& source, Preprocessor::Stage stage) {
	return Preprocessor::process(source, stage, Preprocessor::Options{ Preprocessor::Target::RaylibGlsl330 });
}

// Strips comments, so the checks below only look at code.
std::string codeOnly(const std::string& source) {
	static const std::regex comments(R"(//[^\n]*|/\*[\s\S]*?\*/)");
	return std::regex_replace(source, comments, "");
}

} // namespace

TEST(ShaderPreprocessorRaylib, MatchesGoldenFiles) {
	const auto cases = raylibCases();
	ASSERT_FALSE(cases.empty());
	if (regenerate()) fs::create_directories(kRaylibDir);
	for (const auto& c : cases) {
		const std::string actual = processRaylib(readAll(c.input), c.stage);
		const fs::path expectedPath = kRaylibDir / (c.input.stem().string() + ".expected");
		if (regenerate()) {
			std::ofstream(expectedPath, std::ios::trunc) << actual;
			continue;
		}
		ASSERT_TRUE(fs::exists(expectedPath)) << "missing " << expectedPath;
		EXPECT_EQ(readAll(expectedPath), actual) << c.input.filename();
	}
}

TEST(ShaderPreprocessorRaylib, OutputIsGlsl330Core) {
	static const std::regex legacy(
		R"(\b(gl_TexCoord|gl_Color|gl_FrontColor|gl_FragColor|gl_Vertex|gl_MultiTexCoord0|gl_ModelViewProjectionMatrix|gl_TextureMatrix|attribute|varying|texture2D|texture2DLod|texture2DProj)\b)");
	static const std::regex sampler(R"(\bsampler2D\s+texture\b)");
	for (const auto& c : raylibCases()) {
		const std::string out = processRaylib(readAll(c.input), c.stage);
		EXPECT_EQ(0u, out.rfind("#version 330\n", 0)) << c.input.filename();
		const std::string code = codeOnly(out);
		std::smatch m;
		EXPECT_FALSE(std::regex_search(code, m, legacy)) << c.input.filename() << ": " << m.str();
		EXPECT_FALSE(std::regex_search(code, sampler)) << c.input.filename();
	}
}

TEST(ShaderPreprocessorRaylib, IsIdempotent) {
	for (const auto& c : raylibCases()) {
		const std::string once = processRaylib(readAll(c.input), c.stage);
		EXPECT_EQ(once, processRaylib(once, c.stage)) << c.input.filename();
	}
}

TEST(ShaderPreprocessorRaylib, DefaultStagesAreGlsl330) {
	const auto vs = Preprocessor::defaultVertexShader(Preprocessor::Target::RaylibGlsl330);
	const auto fs = Preprocessor::defaultFragmentShader(Preprocessor::Target::RaylibGlsl330);
	EXPECT_EQ(0u, vs.rfind("#version 330\n", 0));
	EXPECT_EQ(0u, fs.rfind("#version 330\n", 0));
	EXPECT_NE(std::string::npos, vs.find("in vec3 vertexPosition;"));
	EXPECT_NE(std::string::npos, vs.find("out vec4 sf_color;"));
	EXPECT_NE(std::string::npos, fs.find("in vec4 sf_color;"));
	EXPECT_NE(std::string::npos, fs.find("uniform sampler2D texture0;"));
}

TEST(ShaderPreprocessorRaylib, RenamesTheTextureSampler) {
	EXPECT_EQ("texture0", Preprocessor::uniformName("texture", Preprocessor::Target::RaylibGlsl330));
	EXPECT_EQ("noise", Preprocessor::uniformName("noise", Preprocessor::Target::RaylibGlsl330));
	EXPECT_EQ("texture", Preprocessor::uniformName("texture", Preprocessor::Target::SfmlEs100));
}
