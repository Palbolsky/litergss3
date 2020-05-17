#include <iostream>

#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Configuration/DisplayWindowSettings.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"

int main(int argc, char* argv[]) {
	auto eventDispatcher = cgss::MainEventDispatcher{};
	auto settings = cgss::DisplayWindowSettings{};
	settings.video.scale = 1;
	settings.visibleMouse = true;
	settings.video.width = 800;
	settings.video.height = 600;
	settings.title = "Playground";
	auto window = cgss::DisplayWindow{ eventDispatcher, settings };

	auto viewport = window.addView<cgss::Viewport>();
	auto texture = cgss::Texture::create(window, LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
	auto sprite = cgss::Sprite::create(viewport);
	sprite.setTexture(texture);
	sprite.move(100, 100);

	viewport.moveOrigin(100, 100);
	viewport.resize(200, 150);

	auto i = 0;
	{
		auto sprite2 = cgss::Sprite::create(window);
		sprite2.setTexture(texture);
		sprite2.move(200, 300);
		while (window.isOpen()) {
			window.draw();
			window.update();
			sprite2.setAngle(sprite2.getAngle() - 1.f);
			sprite.setAngle(sprite.getAngle() + 4.f);
			if (i > 500) {
				break;
			}
			i++;
		}
	}
	i = 0;
	while (window.isOpen()) {
		window.draw();
		window.update();
		sprite.setAngle(sprite.getAngle() + 1.f);
		if (i == 500) {
			sprite.dispose();
		}
		i++;
	}

	return 0;
}
