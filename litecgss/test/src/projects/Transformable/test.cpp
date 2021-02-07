#include <gtest/gtest.h>
#include "Graphics/Transformable.h"

static bool CheckFloat(float expected, float value) {
	return value >= expected - 0.1f && value <= expected + 0.1f;
}

class TransformableTest : public testing::Test {
protected:
	sf::Transformable instance = {};
	cgss::Transformable transformable = { instance };
};

TEST_F(TransformableTest, Scale) {
	auto checkX = CheckFloat(instance.getScale().x, transformable.getScaleX());
	auto checkY = CheckFloat(instance.getScale().y, transformable.getScaleY());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);

	transformable.scale(4.45f, 12.3f);

	checkX = CheckFloat(4.45f, transformable.getScaleX());
	checkY = CheckFloat(12.3f, transformable.getScaleY());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);
}

TEST_F(TransformableTest, Move) {
	auto checkX = CheckFloat(instance.getPosition().x, transformable.getX());
	auto checkY = CheckFloat(instance.getPosition().y, transformable.getY());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);
	
	transformable.move(4.45f, 12.3f);

	checkX = CheckFloat(4.45f, transformable.getX());
	checkY = CheckFloat(12.3f, transformable.getY());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);
}

TEST_F(TransformableTest, Origin) {
	auto checkX = CheckFloat(instance.getOrigin().x, transformable.getOx());
	auto checkY = CheckFloat(instance.getOrigin().y, transformable.getOy());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);
	
	transformable.moveOrigin(4.45f, 12.3f);

	checkX = CheckFloat(4.45f, transformable.getOx());
	checkY = CheckFloat(12.3f, transformable.getOy());
	EXPECT_TRUE(checkX);
	EXPECT_TRUE(checkY);
}

TEST_F(TransformableTest, Rotate) {
	auto checkAngle = CheckFloat(instance.getRotation(), transformable.getOx());
	EXPECT_TRUE(checkAngle);
	
	transformable.setAngle(4.45f);

	checkAngle = CheckFloat(-355.55f, transformable.getAngle());
	EXPECT_TRUE(checkAngle);
}
