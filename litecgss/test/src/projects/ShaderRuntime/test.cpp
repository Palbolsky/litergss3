// The raylib golden files of the ShaderPreprocessor compiled and linked by the
// real GL driver, through LiteCGSS' own shader loading, on a desktop raylib
// build (the preprocessor passes processed sources through unchanged):
//   - cases/raylib/        Target::RaylibGlsl330 (desktop raylib, GL 3.3)
//   - cases/raylib_es300/  Target::RaylibGlslEs300 (raylib on GL ES 3 /
//                          Android), compiled by a GL 3.3+ driver with
//                          ARB_ES3_compatibility (Mesa)
// Each golden stage is linked with the default of the other stage.
//
// Skipped without a display, on a backend whose shaders aren't raylib's, or
// (per folder) on a driver that doesn't compile its GLSL version.

#include <gtest/gtest.h>

#include <cstdlib>
#include <iterator>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/RenderStatesData.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "Backend/ActiveBackend.h"
#include "Backend/Shader/ShaderPreprocessor.h"

namespace {

namespace fs = std::filesystem;

const fs::path kCasesDir = fs::path(LITE_CGSS_TEST_DIR) / "src" / "projects" / "ShaderPreprocessor" / "cases";

std::string readAll(const fs::path& path) {
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

// Cases of the preprocessor suite that aren't runnable on raylib: vertex
// shaders written for SFML ES (its `position` / `texCoord` attributes and
// sf_* uniforms, which raylib never feeds), left alone by the preprocessor
// because they handle GL ES themselves. They link on desktop GL only because
// it tolerates fragment inputs no vertex output writes; GL ES refuses.
const char* const kNotRunnableOnRaylib[] = {
	"psdk_gles_aware_no_factor_uniform.vert.expected",
	"vertex_stage_passthrough.vert.expected",
};

bool link(const std::string& vertex, const std::string& fragment) {
	cgss::RenderStatesData states;
	return states.loadShaderFromMemory(vertex, fragment);
}

} // namespace

class ShaderRuntime : public ::testing::TestWithParam<const char*> {};

TEST_P(ShaderRuntime, GoldenShadersCompileAndLink) {
	const fs::path kGoldenDir = kCasesDir / GetParam();
	if (!cgss::ShaderPreprocessor::isRaylibTarget(cgss::backend::ActiveBackend::Ops::shader_preprocessor_target())) {
		GTEST_SKIP() << "the backend's shaders aren't raylib's";
	}
	if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr) {
		GTEST_SKIP() << "no display";
	}

	auto settings = cgss::DisplayWindowSettings{};
	settings.video.width = 64;
	settings.video.height = 64;
	settings.video.scale = 1;
	settings.title = "LiteCGSS ShaderRuntime";
	settings.hidden = true;
	auto window = cgss::DisplayWindow{};
	window.reload(std::move(settings), true);

	const std::string defaultVertex = readAll(kGoldenDir / "default.vert.expected");
	const std::string defaultFragment = readAll(kGoldenDir / "default.frag.expected");
	if (!link(defaultVertex, defaultFragment)) {
		GTEST_SKIP() << "this GL driver doesn't compile " << GetParam() << "'s GLSL version (ES 3.00: ARB_ES3_compatibility)";
	}

	std::vector<fs::path> goldens;
	for (const auto& entry : fs::directory_iterator(kGoldenDir)) goldens.push_back(entry.path());
	std::sort(goldens.begin(), goldens.end());
	int linked = 0;
	for (const auto& path : goldens) {
		const std::string name = path.filename().string();
		if (name.rfind("default.", 0) == 0) continue;
		if (std::find(std::begin(kNotRunnableOnRaylib), std::end(kNotRunnableOnRaylib), name) != std::end(kNotRunnableOnRaylib)) continue;
		if (name.find(".frag.") != std::string::npos) {
			EXPECT_TRUE(link(defaultVertex, readAll(path))) << name;
		} else if (name.find(".vert.") != std::string::npos) {
			EXPECT_TRUE(link(readAll(path), defaultFragment)) << name;
		} else {
			continue;
		}
		++linked;
	}
	EXPECT_GT(linked, 0);
}

INSTANTIATE_TEST_SUITE_P(Raylib, ShaderRuntime, ::testing::Values("raylib", "raylib_es300"),
	[](const ::testing::TestParamInfo<const char*>& info) { return std::string(info.param); });
