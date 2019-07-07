#include <iostream>
<<<<<<< 69f86d862365e6a252939bad2943010ca2a09b06
#include "Graphics/Stack/DrawableStack.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"

void CheckDrawableStack() {
	auto stack = lr::View{};
	auto stack2 = lr::View{};

	{
		auto& sprite = lr::Sprite::create(stack);
		auto& sprite2 = lr::Sprite::create(stack);

		std::cout << stack.getSize() << std::endl;

		sprite2.detach();

		std::cout << stack.getSize() << std::endl;

		sprite2.setVisible(false);

		auto& sprite3 = lr::Sprite::create(stack2);

		std::cout << stack.getSize() << " / " << stack2.getSize() << std::endl;
	}

	std::cout << stack2.getSize() << std::endl;

	std::cout << stack.getSize() << std::endl;
}

void CheckDrawableStackWithDrawing() {
	auto view = sf::View{};
	auto renderTarget = sf::RenderWindow{};

	auto stack = lr::View{};
	auto stack2 = lr::View{};

	auto& sprite2 = lr::Sprite::create(stack);
	{
		auto& sprite = lr::Sprite::create(stack);
		

		std::cout << stack.getSize() << std::endl;

		sprite2.detach();

		std::cout << stack.getSize() << std::endl;

		sprite2.setVisible(false);

		auto& sprite3 = lr::Sprite::create(stack2);

		std::cout << stack.getSize() << " / " << stack2.getSize() << std::endl;
	}

	std::cout << stack2.getSize() << std::endl;

	stack.draw(view, renderTarget);
	//Here sprite2 can not be used anymore

	std::cout << stack.getSize() << std::endl;
}

void CheckTextureLoading(const std::string& filename) {
	auto texture = lr::Texture{};
	texture.load(filename, 100, 100, lr::TextureLoadMethod::File);
	auto data = texture.toRawDataPng();
	std::cout << "Texture data size : " << data.size() << std::endl;

	texture.toFile(LITE_CGSS_PLAYGROUND_DIR "/resources/copy.png");
}

int main(int argc, char* argv[]) {
	CheckDrawableStack();
	CheckDrawableStackWithDrawing();
	CheckTextureLoading(LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
=======
#include "Views/Viewport.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"

#include "Views/DisplayWindow.h"
#include "Views/FramedView.h"
#include "Configuration/DisplayWindowSettings.h"

void CheckDrawableStack(lr::MainEventDispatcher& eventDispatcher, const lr::DisplayWindowSettings& settings) {
	
	auto stack = lr::Viewport{ eventDispatcher, settings };
	auto stack2 = lr::Viewport{ eventDispatcher, settings };

	{
		auto& sprite = lr::Sprite::create(stack);
		auto& sprite2 = lr::Sprite::create(stack);

		std::cout << stack.getSize() << std::endl;

		sprite2.detach();

		std::cout << stack.getSize() << std::endl;

		sprite2.setVisible(false);

		auto& sprite3 = lr::Sprite::create(stack2);

		std::cout << stack.getSize() << " / " << stack2.getSize() << std::endl;
	}

	std::cout << stack2.getSize() << std::endl;

	std::cout << stack.getSize() << std::endl;
}

void CheckDrawableStackWithDrawing(lr::MainEventDispatcher& eventDispatcher, const lr::DisplayWindowSettings& settings) {
	auto renderTarget = sf::RenderWindow{};
	auto stack = lr::Viewport{ eventDispatcher, settings };

	auto& sprite2 = lr::Sprite::create(stack);

	sprite2.detach();

	std::cout << stack.getSize() << std::endl;
	sprite2.setVisible(false);

	stack.draw(renderTarget);
	//Here sprite2 can not be used anymore

	std::cout << stack.getSize() << std::endl;
}

void CheckTextureLoading(const std::string& filename) {
	auto texture = lr::Texture{};
	texture.load(filename, 100, 100, lr::TextureLoadMethod::File);
	auto data = texture.toRawDataPng();
	std::cout << "Texture data size : " << data.size() << std::endl;

	texture.toFile(LITE_CGSS_PLAYGROUND_DIR "/resources/copy.png");
}

void CheckCreateWindow(lr::MainEventDispatcher& eventDispatcher, const lr::DisplayWindowSettings& settings) {
	auto window = lr::DisplayWindow{ eventDispatcher, settings };
	window.draw();
}

void CheckWindowEventEmitted(lr::MainEventDispatcher& eventDispatcher, const lr::DisplayWindowSettings& settings) {
	auto window = lr::DisplayWindow{ eventDispatcher, settings };
	window.draw();

	auto event = lr::WindowSettingsChangeEvent{};
	event.settings = settings;
	event.settings.video.scale = 4.0;
	eventDispatcher.lr::Observable<lr::WindowSettingsChangeEvent>::notifyObservers(event);
}

void CheckCreateFramedView(lr::MainEventDispatcher& eventDispatcher, const lr::DisplayWindowSettings& settings) {
	auto window = lr::FramedView{ eventDispatcher, settings };
	window.bindRectangle(nullptr);
}

int main(int argc, char* argv[]) {
	auto settings = lr::DisplayWindowSettings{};
	auto eventDispatcher = lr::MainEventDispatcher{};

	CheckDrawableStack(eventDispatcher, settings);
	CheckDrawableStackWithDrawing(eventDispatcher, settings);
	CheckTextureLoading(LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
	CheckCreateWindow(eventDispatcher, settings);
	CheckCreateFramedView(eventDispatcher, settings);
	CheckWindowEventEmitted(eventDispatcher, settings);

	return 0;
}
