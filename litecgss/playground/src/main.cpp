#include <iostream>
#include "Graphics/Stack/DrawableStack.h"
#include "Graphics/Sprite.h"
#include "Graphics/Texture.h"

void CheckDrawableStack() {
	auto stack = lr::DrawableStack{};
	auto stack2 = lr::DrawableStack{};

	{
		auto sprite = std::make_unique<lr::Sprite>();
		auto sprite2 = std::make_unique<lr::Sprite>();
		stack.add(*sprite);
		stack.add(*sprite2);


		std::cout << stack.size() << std::endl;

		stack.remove(*sprite2);

		std::cout << stack.size() << std::endl;

		stack2.add(*sprite);

		std::cout << stack.size() << " / " << stack2.size() << std::endl;
	}

	std::cout << stack2.size() << std::endl;
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
	CheckTextureLoading(LITE_CGSS_PLAYGROUND_DIR "/resources/test.png");

	return 0;
}
