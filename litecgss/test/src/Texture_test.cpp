#include <doctest.h>

#include "Graphics/Texture.h"
#include "Graphics/Loaders/TextureLoader.h"

TEST_CASE("[Texture] Reading & Writing") {
	auto texture = cgss::Texture{};

	auto loader = cgss::TextureFileLoader { LITE_CGSS_TEST_DIR "/resources/test.png" };
	CHECK(texture.load(loader));

	auto writer = cgss::TextureMemoryLoader{ {} };
	texture.write(writer);
	auto data = writer.stealMemory();

	CHECK(!data.empty());
}
