#ifndef CGSS_SENSOR_TYPE_H
#define CGSS_SENSOR_TYPE_H

// Device sensor enum. Numeric values are ABI (exposed to scripts). Polled
// through Inputs/Sensor.h; backends without sensors report them unavailable.

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
