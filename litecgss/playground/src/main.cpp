#include <iostream>

#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Views/Viewport.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/Shape.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/Sprite.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "LiteCGSS/Backend/ActiveBackend.h"
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
	auto window = cgss::DisplayWindow{ };

	// Desktop-size queries and window-position setters need a live native
	// window under raylib (GetMonitorWidth / SetWindowPosition dispatch to
	// glfw, which asserts on a null handle). Reload first so InitWindow runs,
	// then center. SFML tolerates the reverse order but the window is never
	// actually repositioned there either — so the reordered form is correct
	// on both backends.
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

	auto i = 0;
	{

		while (window.isOpen()) {
			window.draw();
			HandleEvents(window, settings);

			sprite.setAngle(sprite.getAngle() + 1.f);

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
	//MemTest();
	ViewTest();

	return 0;
}
