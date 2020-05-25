#include <iostream>

#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Configuration/DisplayWindowSettings.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"
#include "Graphics/Loaders/TextureLoader.h"

int main(int argc, char* argv[]) {
	auto eventDispatcher = cgss::MainEventDispatcher{};
	auto settings = cgss::DisplayWindowSettings{};
	settings.video.scale = 1;
	settings.visibleMouse = true;
	settings.video.width = 400;
	settings.video.height = 300;
	settings.title = "Playground";
	auto window = cgss::DisplayWindow{ eventDispatcher, settings };

	auto viewport = window.addView<cgss::Viewport>(window);
	auto texture = cgss::Texture::create(window, LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
	auto framedView = window.addView<cgss::FramedView>(viewport);

	auto fillTexture = cgss::Texture{};
	auto memoryLoader = cgss::TextureEmptyLoader{ 1000, 1000 };
	fillTexture.load(memoryLoader);
	fillTexture.fillRect(0, 0, 1000, 1000, sf::Color::White);

	//Move the INSIDE content (i.e. what is drawn), on SOURCE
	framedView.moveOrigin(60, 50);

	//Move the place where it's rendered on screen, on DESTINATION
	framedView.move(100, 100);

	//Resize portion of drawn content, on DESTINATION
	framedView.resize(100, 200);

	auto sprite3 = cgss::Sprite::create(viewport);
	auto sprite4 = cgss::Sprite::create(framedView);
	sprite3.setTexture(texture);
	sprite3.setColor({ 255, 0, 0 });
	//sprite3.move(20, 0);
	sprite3.move(100, 200);	
	
	sprite4.setTexture(fillTexture);
	//sprite4.setZ(0);

	auto sprite = cgss::Sprite::create(framedView);
	sprite.setTexture(texture);
	sprite.move(50, 100);
	sprite.setColor({ 0, 255, 0 });

	/*auto sprite2 = cgss::Sprite::create(window);
	sprite2.setTexture(texture);
	sprite2.move(500, 300);
	sprite2.setColor({255, 128, 128});
	*/

	viewport.moveOrigin(100, 300);
	//viewport.move(50, 50);
	//viewport.resize(400, 300);

	auto i = 0;
	{

		while (window.isOpen()) {
			window.draw();
			//sprite2.setAngle(sprite2.getAngle() - 1.f);
			sprite.setAngle(sprite.getAngle() + 1.f);
			
			if (i == 500) {
				framedView.dispose();
			}

			if (i > 1000) {
				break;
			}
			i++;
		}
	}

	return 0;
}
