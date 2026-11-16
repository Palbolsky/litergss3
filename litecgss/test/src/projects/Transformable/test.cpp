#include <gtest/gtest.h>
#include "LiteCGSS/Graphics/Transformable.h"

static bool CheckFloat(float expected, float value) {
	return value >= expected - 0.1f && value <= expected + 0.1f;
}

// cgss::Transformable owns its own state directly (no backend binding). Its
// default constructor is protected (it's a base class for Sprite/Shape/Text);
// instantiate via a trivial derived class to exercise the API.
struct ConcreteTransformable : public cgss::Transformable {};

class TransformableTest : public testing::Test {
protected:
	ConcreteTransformable transformable {};
};

TEST_F(TransformableTest, Scale) {
	EXPECT_TRUE(CheckFloat(1.f, transformable.getScaleX()));
	EXPECT_TRUE(CheckFloat(1.f, transformable.getScaleY()));

	transformable.scale(4.45f, 12.3f);

	EXPECT_TRUE(CheckFloat(4.45f, transformable.getScaleX()));
	EXPECT_TRUE(CheckFloat(12.3f, transformable.getScaleY()));
}

TEST_F(TransformableTest, Move) {
	EXPECT_TRUE(CheckFloat(0.f, transformable.getX()));
	EXPECT_TRUE(CheckFloat(0.f, transformable.getY()));

	transformable.move(4.45f, 12.3f);

	EXPECT_TRUE(CheckFloat(4.45f, transformable.getX()));
	EXPECT_TRUE(CheckFloat(12.3f, transformable.getY()));
}

TEST_F(TransformableTest, Origin) {
	EXPECT_TRUE(CheckFloat(0.f, transformable.getOx()));
	EXPECT_TRUE(CheckFloat(0.f, transformable.getOy()));

	transformable.moveOrigin(4.45f, 12.3f);

	EXPECT_TRUE(CheckFloat(4.45f, transformable.getOx()));
	EXPECT_TRUE(CheckFloat(12.3f, transformable.getOy()));
}

TEST_F(TransformableTest, Rotate) {
	EXPECT_TRUE(CheckFloat(0.f, transformable.getAngle()));

	transformable.setAngle(4.45f);

	EXPECT_TRUE(CheckFloat(4.45f, transformable.getAngle()));
}
