#ifndef CGSS_TRANSFORMABLE_H
#define CGSS_TRANSFORMABLE_H

namespace cgss {

	// 2D transform state (position, origin, rotation, scale). The state is
	// owned here directly — consumers do not need any backend-specific
	// `setInstance(...)` binding. Each backend reads the state from the
	// cgss::Transformable at draw time (e.g. SFMLBackend::build_transform).
	//
	// Angle is stored as given by the user; the sign convention for the
	// chosen backend is applied inside `build_transform` (SFML flips the
	// sign to match screen-space Y-down rotation direction).
	class Transformable {
	public:
		virtual ~Transformable() = default;

		float getX() const { return m_x; }
		float getY() const { return m_y; }

		float getOx() const { return m_ox; }
		float getOy() const { return m_oy; }

		float getAngle() const { return m_angle; }

		float getScaleX() const { return m_scaleX; }
		float getScaleY() const { return m_scaleY; }

		void move(float x, float y) { m_x = x; m_y = y; }
		void moveOrigin(float ox, float oy) { m_ox = ox; m_oy = oy; }

		void setAngle(float angle) { m_angle = angle; }

		void scale(float xScale, float yScale) { m_scaleX = xScale; m_scaleY = yScale; }

	protected:
		Transformable() = default;

	private:
		float m_x = 0.f;
		float m_y = 0.f;
		float m_ox = 0.f;
		float m_oy = 0.f;
		float m_angle = 0.f;
		float m_scaleX = 1.f;
		float m_scaleY = 1.f;
	};

}

#endif
