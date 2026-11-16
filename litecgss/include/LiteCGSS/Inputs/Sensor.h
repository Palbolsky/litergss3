#ifndef CGSS_INPUTS_SENSOR_H
#define CGSS_INPUTS_SENSOR_H

// Device motion sensors. Backend-neutral (see Inputs/Keyboard.h). Backends
// without sensor support report every sensor as unavailable
// and a zero value.

#include "LiteCGSS/Common/Vector3.h"
#include "LiteCGSS/Events/SensorType.h"

namespace cgss::sensor {

	bool isAvailable(SensorType type);
	void setEnabled(SensorType type, bool enabled);
	Vector3 value(SensorType type);

}

#endif
