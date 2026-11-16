#ifndef CGSS_SENSOR_TYPE_H
#define CGSS_SENSOR_TYPE_H

// Device sensor enum. Numeric values match SFML 2.6 sf::Sensor::Type, so the
// SFML backend casts directly. Polled through Inputs/Sensor.h; backends
// without sensors (raylib) report them unavailable.

namespace cgss {

	enum class SensorType : int {
		Accelerometer     = 0,
		Gyroscope         = 1,
		Magnetometer      = 2,
		Gravity           = 3,
		UserAcceleration  = 4,
		Orientation       = 5,
	};

}

#endif
