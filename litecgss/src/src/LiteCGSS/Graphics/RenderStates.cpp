#include "RenderStates.h"

bool cgss::RenderStates::areGeometryShadersEnabled() {
    return sf::Shader::isGeometryAvailable();
}

bool cgss::RenderStates::areShadersEnabled() {
    return sf::Shader::isAvailable();
}
