#include <iostream>

#include <SFML/Window/Keyboard.hpp>
#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Configuration/DisplayWindowSettings.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"
#include "Graphics/Serializers/TextureSerializer.h"

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
static sf::Vector2i g_windowPosition { 100, 100 };

static void HandleEvents(cgss::DisplayWindow& window, cgss::DisplayWindowSettings& settings) {
	sf::Event event;
	while (window.popEvent(event)) {
		switch(event.type) {
		case sf::Event::EventType::Closed:
			window.stop();
			break;
		case sf::Event::EventType::KeyPressed:
			switch (event.key.code) {
				case sf::Keyboard::F1:
					if (window.screenHeight() > 15 && window.screenWidth() > 20) {
						window.resizeScreen(window.screenWidth() - 20, window.screenHeight() - 15);
						settings.video.height = window.screenHeight();
						settings.video.width = window.screenWidth();
					}
					break;
				case sf::Keyboard::F2:
					window.resizeScreen(window.screenWidth() + 20, window.screenHeight() + 15);
					settings.video.height = window.screenHeight();
					settings.video.width = window.screenWidth();
					break;
				case sf::Keyboard::F3:
					g_disposeView = true;
					break;
				case sf::Keyboard::F5:
					window.move(window.getX() - 10, window.getY() - 10);
					break;
				case sf::Keyboard::F6:
					window.move(window.getX() + 10, window.getY() + 10);
					break;
				case sf::Keyboard::Escape:
					window.stop();
					break;
				case sf::Keyboard::Enter:
					if (event.key.alt) {
						settings.fullscreen = !settings.fullscreen;
						window.reload(settings);
					}
					break;
				default:
				break;
			}
			break;
		case sf::Event::EventType::KeyReleased:
			switch (event.key.code) {
				case sf::Keyboard::Escape:
					g_disposeView = false;
					break;
				default:
				break;
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

	const auto windowPos = sf::Vector2i {
		static_cast<int>(cgss::DisplayWindow::DesktopWidth() - settings.video.width) / 2,
		static_cast<int>(cgss::DisplayWindow::DesktopHeight() - settings.video.height) / 2
	};
	window.move(windowPos.x, windowPos.y);

	window.reload(std::move(settings), true);

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
