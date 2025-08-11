#include "RenderStates.h"

bool cgss::RenderStates::areGeometryShadersEnabled() {
	return cgss::RenderStatesData::enabledShaders() && sf::Shader::isGeometryAvailable();
}

bool cgss::RenderStates::areShadersEnabled() {
	return cgss::RenderStatesData::enabledShaders() && sf::Shader::isAvailable();
}

void cgss::RenderStates::enableShaders(bool enable) {
	cgss::RenderStatesData::enableShaders(enable);
}
