#include "LiteCGSS/Graphics/RenderStates.h"
#include "Backend/ActiveBackend.h"
#include "Backend/Shader/ShaderPreprocessor.h"

namespace { using Ops = cgss::backend::ActiveBackend::Ops; }

bool cgss::RenderStates::areGeometryShadersEnabled() {
	return cgss::RenderStatesData::enabledShaders() && Ops::shader_is_geometry_available();
}

bool cgss::RenderStates::areShadersEnabled() {
	return cgss::RenderStatesData::enabledShaders() && Ops::shader_is_available();
}

void cgss::RenderStates::enableShaders(bool enable) {
	cgss::RenderStatesData::enableShaders(enable);
}

bool cgss::RenderStates::supportsFixedPipeline() {
	return Ops::shader_supports_fixed_pipeline();
}

bool cgss::RenderStates::areShaderPipelineFixed() {
	return Ops::shader_preprocessor_target() != cgss::ShaderPreprocessor::Target::SfmlEs100;
}
