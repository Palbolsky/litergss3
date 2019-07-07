#ifndef CGSS_TRANSFORMABLE_H
#define CGSS_TRANSFORMABLE_H

#include <SFML/Graphics/Transformable.hpp>

namespace cgss {

	class Transformable {
	public:
		~Transformable() = default;
		
		float getX() const;
		float getY() const;

		float getOx() const;
		float getOy() const;

		float getAngle() const;

		float getScaleX() const;
		float getScaleY() const;

		void move(float x, float y);
		void moveOrigin(float ox, float oy);

		void setAngle(float angle);

		void scale(float xScale, float yScale);
	protected:
		Transformable(sf::Transformable& instance);
	private:
		sf::Transformable& m_instance;
	};

}

#endif