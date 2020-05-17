#include <doctest.h>

#include "Graphics/Transformable.h"

static bool CheckFloat(float expected, float value) {
    return value >= expected - 0.1f && value <= expected + 0.1f;
}

TEST_CASE("[Transformable] ") {
    auto instance = sf::Transformable{};
	auto transformable = cgss::Transformable { instance };

    SUBCASE("scale") {
        auto checkX = CheckFloat(instance.getScale().x, transformable.getScaleX());
		auto checkY = CheckFloat(instance.getScale().y, transformable.getScaleY());
        CHECK(checkX);
        CHECK(checkY);
        
        transformable.scale(4.45f, 12.3f);

		checkX = CheckFloat(4.45f, transformable.getScaleX());
		checkY = CheckFloat(12.3f, transformable.getScaleY());
        CHECK(checkX);
        CHECK(checkY);
    }

    SUBCASE("move") {
        auto checkX = CheckFloat(instance.getPosition().x, transformable.getX());
		auto checkY = CheckFloat(instance.getPosition().y, transformable.getY());
        CHECK(checkX);
        CHECK(checkY);
        
        transformable.move(4.45f, 12.3f);

		checkX = CheckFloat(4.45f, transformable.getX());
		checkY = CheckFloat(12.3f, transformable.getY());
        CHECK(checkX);
        CHECK(checkY);
    }

    SUBCASE("move origin") {
        auto checkX = CheckFloat(instance.getOrigin().x, transformable.getOx());
		auto checkY = CheckFloat(instance.getOrigin().y, transformable.getOy());
        CHECK(checkX);
        CHECK(checkY);
        
        transformable.moveOrigin(4.45f, 12.3f);

		checkX = CheckFloat(4.45f, transformable.getOx());
		checkY = CheckFloat(12.3f, transformable.getOy());
        CHECK(checkX);
        CHECK(checkY);
    }

    SUBCASE("rotate") {
        auto checkAngle = CheckFloat(instance.getRotation(), transformable.getOx());
        CHECK(checkAngle);
        
        transformable.setAngle(4.45f);

		checkAngle = CheckFloat(-355.55f, transformable.getAngle());
        CHECK(checkAngle);
    }
}
