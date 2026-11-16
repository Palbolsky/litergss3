#include <chrono>
#ifdef LITECGSS_USE_TRACY
#include <tracy/Tracy.hpp>
#include <client/TracyProfiler.hpp>
#endif
#include <cstring>
#include <iostream>

#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Views/Viewport.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/Font.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/Shape.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/Sprite.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Backend/BackendCommon.h"

#include "LiteCGSS/Events/EventType.h"
#include "LiteCGSS/Events/KeyCode.h"

#include "sample_png.h"

void MemTest() {
	auto settings = cgss::DisplayWindowSettings{};
	settings.video.scale = 1;
	settings.visibleMouse = true;
	settings.video.width = 400;
	settings.video.height = 300;
	settings.title = "MemTest";
	settings.vSync = true;
	auto window = cgss::DisplayWindow{ settings };

	auto viewport = window.addView<cgss::Viewport>();
	auto texture = cgss::Texture::create(window, LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
	viewport.resize(200, 200);

	std::cout << "Before mem alloc" << std::endl;
	for (std::size_t i = 0; i < 1000000; i++) {
		auto sprite = cgss::Sprite::create(viewport);
		sprite.setTexture(texture);
		if (i%10000 == 0) {
			std::cout << (i / 10000.f) <<"%" << std::endl;
		}
	}
	std::cout << "Drawing..." << std::endl;
	window.draw();
	std::cout << "Before mem release" << std::endl;
	viewport.detach();
	std::cout << "After mem release" << std::endl;
}

// Loads a TTF from a short probe list of common system locations.
// cgss::Font's public API only exposes filename-based load, so we pick
// the first hit. Leaves the font unloaded if nothing matches — callers
// must gate ScrollText wiring on isLoaded() to stay portable on hosts
// without these paths.
static cgss::Font loadDemoFont() {
	auto font = cgss::Font{};
	static constexpr const char* candidates[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/share/fonts/dejavu/DejaVuSans.ttf",
		"/Library/Fonts/Arial.ttf",
		"C:\\Windows\\Fonts\\arial.ttf",
	};
	for (const auto* path : candidates) {
		if (font.load(path)) return font;
	}
	return font;
}

static bool g_disposeView = false;

static void HandleEvents(cgss::DisplayWindow& window, cgss::DisplayWindowSettings& settings) {
	using Ops = cgss::backend::ActiveBackend::Ops;
	using cgss::backend::native_of;

	cgss::Event event;
	while (window.popEvent(event)) {
		const auto& native = native_of(event);
		switch (Ops::event_type(native)) {
		case cgss::EventType::Closed:
			window.stop();
			break;
		case cgss::EventType::KeyPressed:
			if (auto key = Ops::event_as_key_press(native)) {
				switch (key->code) {
				case cgss::KeyCode::F1:
					if (window.screenHeight() > 15 && window.screenWidth() > 20) {
						window.resizeScreen(window.screenWidth() - 20, window.screenHeight() - 15);
						settings.video.height = window.screenHeight();
						settings.video.width  = window.screenWidth();
					}
					break;
				case cgss::KeyCode::F2:
					window.resizeScreen(window.screenWidth() + 20, window.screenHeight() + 15);
					settings.video.height = window.screenHeight();
					settings.video.width  = window.screenWidth();
					break;
				case cgss::KeyCode::F3:
					g_disposeView = true;
					break;
				case cgss::KeyCode::F5:
					window.move(window.getX() - 10, window.getY() - 10);
					break;
				case cgss::KeyCode::F6:
					window.move(window.getX() + 10, window.getY() + 10);
					break;
				case cgss::KeyCode::Escape:
					window.stop();
					break;
				case cgss::KeyCode::Enter:
					if (key->alt) {
						settings.fullscreen = !settings.fullscreen;
						window.reload(settings);
					}
					break;
				default:
					break;
				}
			}
			break;
		case cgss::EventType::KeyReleased:
			if (auto key = Ops::event_as_key_release(native)) {
				if (key->code == cgss::KeyCode::Escape) g_disposeView = false;
			}
			break;
		default:
			break;
		}
	}
}

static void ViewTest() {
	auto settings = cgss::DisplayWindowSettings{};
	settings.video.scale = 1;
	settings.visibleMouse = true;
	settings.video.width = 400;
	settings.video.height = 300;
	settings.title = "Playground";
	settings.vSync = true;
	auto window = cgss::DisplayWindow{ };

	// Desktop-size queries and window-position setters may need a live
	// native window (some backends assert on a null window handle). Reload
	// first so the window exists, then center — correct on every backend.
	const auto videoWidth  = settings.video.width;
	const auto videoHeight = settings.video.height;
	window.reload(std::move(settings), true);

	const auto windowPos = cgss::backend::Point2i {
		static_cast<int>(cgss::DisplayWindow::DesktopWidth() - videoWidth) / 2,
		static_cast<int>(cgss::DisplayWindow::DesktopHeight() - videoHeight) / 2
	};
	window.move(windowPos.x, windowPos.y);

	auto viewport = window.addView<cgss::Viewport>();
	auto serializer = cgss::TextureMemorySerializer{ {g_sample_png, g_sample_png_length}, false };
	auto texture = cgss::Texture{};
	if (!texture.load(serializer)) {
		throw std::runtime_error("Failed to load bitmap from memory.");
	}
	auto framedView = viewport.addView<cgss::FramedView>();

	auto fillTexture = cgss::Texture{};
	auto memorySerializer = cgss::TextureEmptySerializer{ 1000, 1000 };
	fillTexture.load(memorySerializer);
	fillTexture.fillRect(0, 0, 1000, 1000, cgss::Colors::White);

	//Move the INSIDE content (i.e. what is drawn), on SOURCE
	framedView.moveOrigin(60, 50);

	//Move the place where it's rendered on screen, on DESTINATION
	framedView.move(100, 100);

	//Resize portion of drawn content, on DESTINATION
	framedView.resize(100, 200);

	auto sprite3 = cgss::Sprite::create(viewport);
	auto sprite4 = cgss::Sprite::create(framedView);
	sprite3.setTexture(texture);
	sprite3.setColor({ 255, 120, 120 });
	//sprite3.move(20, 0);
	//sprite3.move(50, 100);

	sprite4.setTexture(fillTexture);
	//sprite4.setZ(0);

	auto sprite = cgss::Sprite::create(framedView);
	sprite.setTexture(texture);
	sprite.move(50, 100);
	sprite.setColor({ 120, 255, 120 });

	auto sprite2 = cgss::Sprite::create(window);
	sprite2.setTexture(texture);
	sprite2.move(200, 100);
	sprite2.setColor({128, 128, 255});

	// Rectangle shape on the viewport — filled yellow with a red outline.
	// Draws behind the sprites because Drawable Z defaults to 0 and sprites
	// were added earlier (stable iteration order).
	cgss::ShapeGeometry rectGeom;
	rectGeom.type = cgss::ShapeType::Rectangle;
	rectGeom.width = 80.f;
	rectGeom.height = 60.f;
	rectGeom.fillColor = cgss::Color{255, 220, 80, 255};
	rectGeom.outlineColor = cgss::Color{200, 40, 40, 255};
	rectGeom.outlineThickness = 3.f;
	auto rectShape = cgss::Shape::create(viewport, rectGeom);
	rectShape.move(260, 190);

	// Circle shape on the window level — filled cyan with a black outline.
	cgss::ShapeGeometry circleGeom;
	circleGeom.type = cgss::ShapeType::Circle;
	circleGeom.radius = 30.f;
	circleGeom.pointCount = 32;
	circleGeom.fillColor = cgss::Color{60, 200, 220, 255};
	circleGeom.outlineColor = cgss::Color{20, 20, 30, 255};
	circleGeom.outlineThickness = 2.f;
	auto circleShape = cgss::Shape::create(window, circleGeom);
	circleShape.move(310, 40);

	viewport.move(50, 50);

	// ScrollText smoke test — exercises the active backend's ScrollText
	// through the backend-neutral cgss::Text API. When no system
	// font is found (isLoaded() == false), the whole block is skipped so
	// the playground still runs on hosts without any of the probe-list
	// fonts.
	auto demoFont = loadDemoFont();
	cgss::Text titleText;
	cgss::Text typewriterText;
	cgss::Text framedText;
	cgss::Text outlineText;
	cgss::Text shadowText;
	cgss::Text boldUnderlinedText;
	cgss::Text strikeText;
	// ScrollText style bits are shared by every backend's ScrollText, so the
	// uint32 mask is portable.
	static constexpr std::uint32_t kStyleBold          = 1u << 0;
	static constexpr std::uint32_t kStyleUnderlined    = 1u << 2;
	static constexpr std::uint32_t kStyleStrikeThrough = 1u << 3;
	static constexpr const char* kTypewriterPhrase =
		"Hello from the playground! This line types itself.";

	if (demoFont.isLoaded()) {
		// Static label — window level, drawn over everything else.
		titleText = cgss::Text::create(window);
		titleText.setFont(demoFont);
		titleText.setCharacterSize(18);
		titleText.setFillColor(cgss::Colors::White);
		titleText.setString("LiteCGSS ScrollText");
		titleText.move(8, 8);

		// Typewriter — window level, bottom. numCharToDraw is advanced in
		// the loop below to show the UTF-8 truncation path.
		typewriterText = cgss::Text::create(window);
		typewriterText.setFont(demoFont);
		typewriterText.setCharacterSize(14);
		typewriterText.setFillColor(cgss::Color{ 255, 240, 160, 255 });
		typewriterText.setString(kTypewriterPhrase);
		typewriterText.setNumCharToDraw(0);
		typewriterText.move(8, 270);

		// FramedView text — verifies text renders correctly inside a view
		// with its own origin offset and clipped region.
		framedText = cgss::Text::create(framedView);
		framedText.setFont(demoFont);
		framedText.setCharacterSize(16);
		framedText.setFillColor(cgss::Color{ 40, 40, 80, 255 });
		framedText.setString("Framed!");
		framedText.move(70, 60);

		// Phase 2 showcase — each of these exercises a distinct style path:
		//   outline      → 8-tap stamp in outline color
		//   shadow       → 3-stamp bottom-right
		//   bold+under   → synthetic bold re-stamp + underline rect
		//   strike       → strikethrough rect mid-glyph
		outlineText = cgss::Text::create(window);
		outlineText.setFont(demoFont);
		outlineText.setCharacterSize(18);
		outlineText.setFillColor(cgss::Color{ 255,  80,  80, 255 });
		outlineText.setOutlineColor(cgss::Color{  20,  20,  20, 255 });
		outlineText.setOutlineThickness(1.5f);
		outlineText.setDrawShadow(false);
		outlineText.setString("outline");
		outlineText.move(8, 30);

		shadowText = cgss::Text::create(window);
		shadowText.setFont(demoFont);
		shadowText.setCharacterSize(18);
		shadowText.setFillColor(cgss::Color{ 220, 240, 255, 255 });
		shadowText.setOutlineColor(cgss::Color{  0,   0,   0,  220 });
		shadowText.setDrawShadow(true);
		shadowText.setString("shadow");
		shadowText.move(120, 30);

		boldUnderlinedText = cgss::Text::create(window);
		boldUnderlinedText.setFont(demoFont);
		boldUnderlinedText.setCharacterSize(16);
		boldUnderlinedText.setFillColor(cgss::Color{ 180, 255, 180, 255 });
		boldUnderlinedText.setStyle(kStyleBold | kStyleUnderlined);
		boldUnderlinedText.setDrawShadow(false);
		boldUnderlinedText.setString("bold + underlined");
		boldUnderlinedText.move(8, 55);

		strikeText = cgss::Text::create(window);
		strikeText.setFont(demoFont);
		strikeText.setCharacterSize(16);
		strikeText.setFillColor(cgss::Color{ 255, 180, 180, 255 });
		strikeText.setStyle(kStyleStrikeThrough);
		strikeText.setDrawShadow(false);
		strikeText.setString("struck-through");
		strikeText.move(190, 55);
	} else {
		std::cout << "No system font found in probe list; ScrollText demo skipped." << std::endl;
	}

	// Shader demo — bind a fragment shader to the viewport's RenderStates so
	// every drawable rendered through the viewport (framedView + sprite3 +
	// rectShape + framedText) goes through the post-process. Window-level
	// drawables (sprite2, circleShape, ScrollTexts) bypass it. Uses the
	// legacy GLSL 110 entry shape (gl_TexCoord/gl_FragColor) so the same
	// source compiles unchanged via cgss::ShaderType::Fragment. The u_time
	// uniform is updated every frame inside the loop — wave + grayscale pulse.
	static constexpr const char* kViewportFragmentShader =
		"uniform sampler2D texture;"
		"uniform float u_time;"
		"void main() {"
		"  vec2 uv = gl_TexCoord[0].xy;"
		"  uv.x += 0.01 * sin(u_time * 2.0 + uv.y * 12.0);"
		"  vec4 c = texture2D(texture, uv);"
		"  float gray = dot(c.rgb, vec3(0.299, 0.587, 0.114));"
		"  float k = 0.5 + 0.5 * sin(u_time);"
		"  vec3 col = mix(c.rgb, vec3(gray), k);"
		"  gl_FragColor = vec4(col, c.a) * gl_Color;"
		"}";

	cgss::RenderStates viewportShader;
	const bool shaderLoaded = viewportShader.data().loadShaderFromMemory(
		kViewportFragmentShader, cgss::ShaderType::Fragment);
	if (shaderLoaded) {
		viewport.bindRenderStates(&viewportShader);
	} else {
		std::cout << "Viewport shader failed to compile; demo continues unshaded." << std::endl;
	}

	auto i = 0;
	{
		// Backend-agnostic FPS counter + cumulative iteration count. FPS shows
		// instantaneous pacing; (totalIters / totalSeconds) shows long-term
		// average rate — if the loop is truly pegged at 60 Hz, the two must
		// agree and stay stable across backends.
		using clock = std::chrono::steady_clock;
		const auto loopStart = clock::now();
		auto fpsWindowStart  = loopStart;
		auto fpsFrames       = 0u;

		while (window.isOpen()) {
			window.draw();
			HandleEvents(window, settings);

			++fpsFrames;
			const auto now = clock::now();
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsWindowStart);
			if (elapsed.count() >= 1000) {
				const double fps = fpsFrames * 1000.0 / static_cast<double>(elapsed.count());
				const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - loopStart).count();
				const double avgRate = (i + 1) * 1000.0 / static_cast<double>(totalMs);
				std::cout << "FPS: " << fps
				          << "  i=" << (i + 1)
				          << "  t=" << (totalMs / 1000.0) << "s"
				          << "  avg=" << avgRate << " it/s"
				          << std::endl;
				fpsWindowStart = now;
				fpsFrames      = 0;
			}

			sprite.setAngle(sprite.getAngle() + 1.f);

			if (shaderLoaded) {
				const float t = std::chrono::duration<float>(now - loopStart).count();
				viewportShader.data().setShaderUniform("u_time", t);
			}

			// Advance the typewriter one codepoint every ~4 frames, then
			// pause on the full string for ~60 frames before restarting.
			if (demoFont.isLoaded()) {
				const auto total = static_cast<std::uint32_t>(std::strlen(kTypewriterPhrase));
				const auto cycle = (total + 60u) * 4u;
				const auto frame = static_cast<std::uint32_t>(i) % cycle;
				const auto progress = frame / 4u;
				typewriterText.setNumCharToDraw(progress > total ? total : progress);
			}

			if (g_disposeView) {
				framedView.detach();
			}

			i++;
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
#ifdef LITECGSS_USE_TRACY
	// TRACY_MANUAL_LIFETIME: the host starts the profiler before LiteCGSS runs.
	tracy::StartupProfiler();
#endif
	//MemTest();
	ViewTest();
#ifdef LITECGSS_USE_TRACY
	tracy::ShutdownProfiler();
#endif

	return 0;
}
