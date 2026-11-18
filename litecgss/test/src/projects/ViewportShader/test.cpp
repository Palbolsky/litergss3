// A viewport with a shader (PSDK's map viewports) on a window larger than the
// game (an Android host's letterbox: an origin, a scale): its render texture
// is cleared every frame, so what it drew once is gone once it no longer
// draws it.
//
// The viewport draws into the window's shared render texture, of the game's
// size, then through its shader onto the window. raylib computes a scissor
// against the bound framebuffer: the view clipping the viewport, installed
// before the render texture was bound, got the window's (taller at scale 2),
// its scissor missed the texture, and the clear cleared nothing. The texture
// kept its last content: on Android, PSDK's empty top map viewport showed
// fragments of former frames over the game.
//
// The texture was also read back at the wrong rows (texH - top instead of
// texH - top - height): right only for a game covering the whole window.
//
// The presented frame is read through the frame dump (LITECGSS_DUMP_FRAMES,
// before the buffers swap): skipped where it isn't compiled (LITECGSS_AGENT),
// or without a display.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/RenderStatesData.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "LiteCGSS/Graphics/Sprite.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Image/Image.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Views/Viewport.h"
#include "Backend/ActiveBackend.h"
#include "Backend/Shader/ShaderPreprocessor.h"

namespace {

namespace fs = std::filesystem;

// The game: 64x64 at (16, 16) on a 96x96 window, scaled by 2
constexpr long kOrigin = 16;
constexpr unsigned int kWindowSize = 96;
constexpr unsigned int kScale = 2;
constexpr int kGameSize = 64;

const fs::path kShadersDir = fs::path(LITE_CGSS_TEST_DIR) / "src" / "projects" / "ShaderPreprocessor" / "cases" / "raylib";

std::string readAll(const fs::path& path) {
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

cgss::Texture filledTexture(unsigned int size, cgss::Color color) {
	auto texture = cgss::Texture{};
	auto empty = cgss::TextureEmptySerializer{size, size};
	texture.load(empty);
	texture.fillRect(0, 0, size, size, color);
	return texture;
}

// The frame dumped last: frames are numbered (f<frame>-...)
std::optional<cgss::Image> lastFrame(const fs::path& dir) {
	std::vector<fs::path> frames;
	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.path().extension() == ".png") frames.push_back(entry.path());
	}
	if (frames.empty()) return std::nullopt;
	std::sort(frames.begin(), frames.end());
	return cgss::Image::create(frames.back().string());
}

// A game pixel's color in the presented frame
cgss::Color gamePixel(const cgss::Image& frame, int x, int y) {
	const auto px = static_cast<unsigned int>((kOrigin + x) * kScale);
	const auto py = static_cast<unsigned int>((kOrigin + y) * kScale);
	return frame.getPixel(px, py).value_or(cgss::Color{ 0, 0, 0, 0 });
}

bool isRed(cgss::Color c) { return c.r > 180 && c.g < 80 && c.b < 80; }
bool isBlue(cgss::Color c) { return c.b > 180 && c.r < 80 && c.g < 80; }

} // namespace

TEST(ViewportShader, ClearedEveryFrameOnALetterboxedScaledWindow) {
	if (!cgss::ShaderPreprocessor::isRaylibTarget(cgss::backend::ActiveBackend::Ops::shader_preprocessor_target())) {
		GTEST_SKIP() << "the backend's shaders aren't raylib's";
	}
	if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr) {
		GTEST_SKIP() << "no display";
	}
	// Before the first frame: the dump reads its configuration once
	const fs::path dumpDir = fs::temp_directory_path() / ("litecgss-viewport-shader-" + std::to_string(::getpid()));
	fs::remove_all(dumpDir);
	const std::string dumpSpec = dumpDir.string() + ":every=1";
	::setenv("LITECGSS_DUMP_FRAMES", dumpSpec.c_str(), 1);

	auto settings = cgss::DisplayWindowSettings{};
	settings.video.width = kWindowSize;
	settings.video.height = kWindowSize;
	settings.video.scale = kScale;
	settings.title = "LiteCGSS ViewportShader";
	settings.hidden = true;
	auto window = cgss::DisplayWindow{};
	window.reload(std::move(settings), true);
	window.setOrigin(kOrigin, kOrigin);

	// Blue background, no shader
	auto background = window.addView<cgss::Viewport>();
	background.setRectangle(cgss::IntRect{ 0, 0, kGameSize, kGameSize });
	auto blue = filledTexture(kGameSize, cgss::Color{ 40, 60, 220, 255 });
	auto backgroundSprite = cgss::Sprite::create(background);
	backgroundSprite.setTexture(blue, true);

	// Above it, a viewport with a shader (raylib's default: draws its texture
	// as is) and a red square
	auto shaded = window.addView<cgss::Viewport>();
	shaded.setRectangle(cgss::IntRect{ 0, 0, kGameSize, kGameSize });
	auto renderStates = cgss::RenderStates{};
	ASSERT_TRUE(renderStates.data().loadShaderFromMemory(
		readAll(kShadersDir / "default.vert.expected"), readAll(kShadersDir / "default.frag.expected")));
	shaded.bindRenderStates(&renderStates);
	auto red = filledTexture(32, cgss::Color{ 220, 40, 40, 255 });
	auto square = cgss::Sprite::create(shaded);
	square.setTexture(red, true);
	square.move(8.f, 8.f);

	window.draw();
	auto first = lastFrame(dumpDir);
	if (!first) {
		fs::remove_all(dumpDir);
		GTEST_SKIP() << "no frame dump (LITECGSS_AGENT off)";
	}
	// The square at (8, 8)-(40, 40), where it is: the shaded viewport's
	// texture read back at the game's place in it (its origin), not the rows
	// past it
	EXPECT_TRUE(isRed(gamePixel(*first, 20, 10))) << "the square's top, through the shaded viewport";
	EXPECT_TRUE(isRed(gamePixel(*first, 20, 37))) << "the square's bottom";
	EXPECT_TRUE(isBlue(gamePixel(*first, 20, 44))) << "the background below it";
	EXPECT_TRUE(isBlue(gamePixel(*first, 50, 50))) << "the background beside it";

	// The square goes: the shaded viewport draws nothing, its texture must
	// show nothing either
	square.setVisible(false);
	window.draw();
	auto second = lastFrame(dumpDir);
	ASSERT_TRUE(second);
	const auto left = gamePixel(*second, 20, 20);
	EXPECT_TRUE(isBlue(left)) << "the square is still shown: rgba(" << int(left.r) << ", " << int(left.g)
		<< ", " << int(left.b) << ", " << int(left.a) << ")";

	shaded.bindRenderStates(nullptr);
	fs::remove_all(dumpDir);
}
