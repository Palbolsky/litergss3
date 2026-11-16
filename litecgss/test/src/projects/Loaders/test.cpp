#include <gtest/gtest.h>
#include "LiteCGSS/Graphics/Serializers/ShapeLoader.h"

TEST(LoaderGroup, Shape) {
	const auto circleShape = cgss::ShapeLoader::buildCircle(5.4f, 1);
	EXPECT_EQ(circleShape.type, cgss::ShapeType::Circle);

	const auto convexShape = cgss::ShapeLoader::buildConvex(2);
	EXPECT_EQ(convexShape.type, cgss::ShapeType::Convex);

	const auto rectangleShape = cgss::ShapeLoader::buildRectangle(3.1f, 3);
	EXPECT_EQ(rectangleShape.type, cgss::ShapeType::Rectangle);
}

/*
  Needs X11...

TEST(LoaderGroup, Texture) {
	auto textureSerializer = cgss::TextureFileSerializer {"123.png"};
	auto texture = cgss::backend::ActiveBackend::native_texture{};
	auto image = cgss::backend::ActiveBackend::native_image{};
	textureSerializer.load(texture, image);
}
*/
