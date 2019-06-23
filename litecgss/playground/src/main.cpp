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

void CheckDrawableStack() {
	auto stack = lr::Viewport{};
	auto stack2 = lr::Viewport{};

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
	auto renderTarget = sf::RenderWindow{};

	auto stack = lr::Viewport{};

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

int main(int argc, char* argv[]) {
<<<<<<< 69f86d862365e6a252939bad2943010ca2a09b06
	auto viewport = lr::Viewport {};
	auto window = lr::DisplayWindow {};
	
	auto& sh1 = lr::Shape::create(viewport);
	auto& sh2 = lr::Shape::create(window);
>>>>>>> feature : EXV01 compile-time implementation
=======
	CheckDrawableStack();
	CheckDrawableStackWithDrawing();
	CheckTextureLoading(LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");
>>>>>>> tests : revert unintended modifications

	return 0;
}
