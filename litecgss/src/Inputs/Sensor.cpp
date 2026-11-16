#include "LiteCGSS/Inputs/Sensor.h"

#include "Backend/ActiveBackend.h"

using Ops = cgss::backend::ActiveBackend::Ops;

bool cgss::sensor::isAvailable(SensorType type) {
	return Ops::sensor_is_available(type);
}

void cgss::sensor::setEnabled(SensorType type, bool enabled) {
	Ops::sensor_set_enabled(type, enabled);
}

cgss::Vector3 cgss::sensor::value(SensorType type) {
	return Ops::sensor_get_value(type);
}
