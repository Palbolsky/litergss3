#include <gtest/gtest.h>
#include "Graphics/Loaders/ShapeLoader.h"
#include "Graphics/Loaders/TextureLoader.h"

TEST(LoaderGroup, Shape) {
	const auto circleShape = cgss::ShapeLoader::buildCircle(5.4f, 1);
	EXPECT_EQ(circleShape.first, cgss::ShapeType::Circle);

	const auto convexShape = cgss::ShapeLoader::buildConvex(2);
	EXPECT_EQ(convexShape.first, cgss::ShapeType::Convex);

	const auto rectangleShape = cgss::ShapeLoader::buildRectangle(3.1f, 3);
	EXPECT_EQ(rectangleShape.first, cgss::ShapeType::Rectangle);
}

/*
  Needs X11...

TEST(LoaderGroup, Texture) {
	auto textureLoader = cgss::TextureFileLoader {"123.png"};
	auto texture = sf::Texture{};
	auto image = sf::Image{};
	textureLoader.load(texture, image);
}
*/