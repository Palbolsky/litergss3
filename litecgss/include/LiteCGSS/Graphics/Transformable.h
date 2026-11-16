#ifndef CGSS_TRANSFORMABLE_H
#define CGSS_TRANSFORMABLE_H

#include <cstdint>

namespace cgss {

	// 2D transform state (position, origin, rotation, scale). The state is
	// owned here directly — consumers do not need any backend-specific
	// `setInstance(...)` binding. Each backend reads the state from the
	// cgss::Transformable at draw time (Ops::build_transform).
	//
	// Angle is stored as given by the user; the sign convention for the
	// chosen backend is applied inside `build_transform` (e.g. flipping the
	// sign to match screen-space Y-down rotation direction).
	//
	// `m_version` is bumped by every setter so callers (Sprite/Shape items)
	// can cache a backend-native matrix derived from this state and rebuild
	// only when the version they saw last differs.
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

		std::uint32_t transformVersion() const { return m_version; }

		void move(float x, float y) { m_x = x; m_y = y; ++m_version; }
		void moveOrigin(float ox, float oy) { m_ox = ox; m_oy = oy; ++m_version; }

		void setAngle(float angle) { m_angle = angle; ++m_version; }

		void scale(float xScale, float yScale) { m_scaleX = xScale; m_scaleY = yScale; ++m_version; }

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
		std::uint32_t m_version = 1;
	};

}

#endif
