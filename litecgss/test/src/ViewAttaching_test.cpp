#include <doctest.h>

#include "Views/DisplayWindow.h"
#include "Views/Viewport.h"
#include "Graphics/Sprite.h"

TEST_CASE("[ViewAttaching] ") {
    auto eventDispatcher = cgss::MainEventDispatcher{};
	auto settings = cgss::DisplayWindowSettings{};
	settings.headless = true;

	auto window = cgss::DisplayWindow{ eventDispatcher, settings };

	auto viewport = window.addView<cgss::Viewport>(window);
	auto texture = cgss::Texture{};
	texture.create(20, 30);
	auto framedView = window.addView<cgss::FramedView>(viewport);

	auto sprite = cgss::Sprite::create(viewport);
	sprite.setTexture(texture);

	auto sprite3 = cgss::Sprite::create(framedView);
	sprite3.setTexture(texture);

	viewport.moveOrigin(100, 100);
}

