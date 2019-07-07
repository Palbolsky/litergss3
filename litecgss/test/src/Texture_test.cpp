#include <doctest.h>

#include "Graphics/Texture.h"

TEST_CASE("[Texture] Reading & Writing") {
	auto texture = cgss::Texture{};
	texture.load(LITE_CGSS_TEST_DIR "/resources/test.png", 100, 100, cgss::TextureLoadMethod::File);
	auto data = texture.toRawDataPng();
	//std::cout << "Texture data size : " << data.size() << std::endl;

	texture.toFile(LITE_CGSS_TEST_DIR "/resources/copy.png");	
	//TODO check file exists
}
