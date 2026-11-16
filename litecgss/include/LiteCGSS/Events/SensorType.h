#ifndef CGSS_SENSOR_TYPE_H
#define CGSS_SENSOR_TYPE_H

// Placeholder sensor enum. Numeric values match SFML 2.6 sf::Sensor::Type.
// Neither the current SFML nor raylib backends implement sensor ops —
// this header reserves the public layout for future backends (libretro,
// mobile) without needing an ABI break later.

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
