#include <iostream>
#include "Common/Profiling.h"

#include "LiteCGSS/Graphics/RenderStatesData.h"
#include "Backend/ActiveBackend.h"
#include "Backend/Shader/ShaderPreprocessor.h"
#include "LiteCGSS/Graphics/Texture.h"

// Every shader source goes through cgss::ShaderPreprocessor (NPOT handling,
// and the fixed-pipeline built-ins rewritten for backends that don't feed
// them), for the target the active backend reports.

namespace {
	using Preprocessor = cgss::ShaderPreprocessor;
	using UniformDefaults = std::vector<Preprocessor::UniformDefault>;

	Preprocessor::Stage stageFromShaderType(cgss::ShaderType type) {
		switch (type) {
			case cgss::ShaderType::Vertex:   return Preprocessor::Stage::Vertex;
			case cgss::ShaderType::Geometry: return Preprocessor::Stage::Geometry;
			case cgss::ShaderType::Fragment: // fall through
			default:                         return Preprocessor::Stage::Fragment;
		}
	}

	template <class B>
	Preprocessor::Result preprocess(const std::string& source, Preprocessor::Stage stage) {
		return Preprocessor::processDetailed(source, stage, Preprocessor::Options{ B::Ops::shader_preprocessor_target() });
	}

	// Gives the uniforms the values their declarations held (removed by the
	// preprocessor on GL ES, which forbids them).
	template <class B>
	void applyUniformDefaults(typename B::native_shader& shader, const UniformDefaults& defaults) {
		for (const auto& d : defaults) {
			const auto& v = d.values;
			if (d.type == "float")     B::Ops::shader_set_uniform(shader, d.name, v[0]);
			else if (d.type == "vec2") B::Ops::shader_set_uniform(shader, d.name, cgss::Glsl::Vec2{ v[0], v[1] });
			else if (d.type == "vec3") B::Ops::shader_set_uniform(shader, d.name, cgss::Glsl::Vec3{ v[0], v[1], v[2] });
			else if (d.type == "vec4") B::Ops::shader_set_uniform(shader, d.name, cgss::Glsl::Vec4{ v[0], v[1], v[2], v[3] });
			else if (d.type == "int")  B::Ops::shader_set_uniform(shader, d.name, static_cast<int>(v[0]));
			else if (d.type == "bool") B::Ops::shader_set_uniform(shader, d.name, v[0] != 0.f);
		}
	}

	UniformDefaults concat(UniformDefaults a, const UniformDefaults& b) {
		a.insert(a.end(), b.begin(), b.end());
		return a;
	}
}

template <class B> bool cgss::basic_RenderStatesData<B>::SHADERS_ENABLED = true;

template <class B>
cgss::basic_RenderStatesData<B>::basic_RenderStatesData() :
	m_shadersEnabled(true) {
}

template <class B> cgss::basic_RenderStatesData<B>::basic_RenderStatesData(basic_RenderStatesData&&) = default;
template <class B> cgss::basic_RenderStatesData<B>& cgss::basic_RenderStatesData<B>::operator=(basic_RenderStatesData&&) = default;
template <class B> cgss::basic_RenderStatesData<B>::~basic_RenderStatesData() = default;

template <class B>
void cgss::basic_RenderStatesData<B>::enableShaders(bool enable) {
	SHADERS_ENABLED = enable;
}

template <class B>
bool cgss::basic_RenderStatesData<B>::enabledShaders() {
	return SHADERS_ENABLED;
}

template <class B>
void cgss::basic_RenderStatesData<B>::warnIfShadersNotAvailable() {
	// Two distinct disable paths — surface both so the cause is unambiguous:
	//   - SHADERS_ENABLED == false: set by the host (Ruby:
	//     `LiteRGSS::Shader.available = false`) and process-wide; a loader
	//     disabling shaders and never restoring them silently breaks every
	//     later shader load.
	//   - the backend can't compile shaders at all (no GLSL support).
	if (!SHADERS_ENABLED) {
		std::cerr << "LiteCGSS: shader load attempted while SHADERS_ENABLED == false. "
		             "Some Ruby code called `LiteRGSS::Shader.available = false` and never restored it. "
		             "Until restored, every Shader.new returns a non-functional shader and sprites "
		             "fall back to the default sampler shader (you'll see the bitmap, not your shader output)."
		          << std::endl;
		return;
	}
	if (!B::Ops::shader_is_available()) {
		std::cerr << "LiteCGSS: shader load attempted but the graphics backend reports shaders as unavailable. "
		             "The current GL context can't compile shaders (no GLSL support reported by the driver)."
		          << std::endl;
	}
}

template <class B>
void cgss::basic_RenderStatesData<B>::updateShader() {
	if (!m_shadersEnabled) {
		m_shader = nullptr;
	}
	B::Ops::draw_state_set_shader(cgss::backend::native_of(m_drawState), m_shader.get());
}

template <class B>
bool cgss::basic_RenderStatesData<B>::isAvailable() {
	return SHADERS_ENABLED && B::Ops::shader_is_available();
}

template <class B>
void cgss::basic_RenderStatesData<B>::enableShader(bool enable) {
	m_shadersEnabled = enable && isAvailable();
	updateShader();
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendColorSrcFactor(cgss::BlendFactor colorSrcFactor) {
	B::Ops::draw_state_set_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::ColorSrc, colorSrcFactor);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendColorDstFactor(cgss::BlendFactor colorDstFactor) {
	B::Ops::draw_state_set_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::ColorDst, colorDstFactor);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendAlphaSrcFactor(cgss::BlendFactor alphaSrcFactor) {
	B::Ops::draw_state_set_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::AlphaSrc, alphaSrcFactor);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendAlphaDstFactor(cgss::BlendFactor alphaDstFactor) {
	B::Ops::draw_state_set_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::AlphaDst, alphaDstFactor);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendColorEquation(cgss::BlendEquation colorEquation) {
	B::Ops::draw_state_set_blend_equation(cgss::backend::native_of(m_drawState), cgss::backend::BlendEquationSlot::Color, colorEquation);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setBlendAlphaEquation(cgss::BlendEquation alphaEquation) {
	B::Ops::draw_state_set_blend_equation(cgss::backend::native_of(m_drawState), cgss::backend::BlendEquationSlot::Alpha, alphaEquation);
}

template <class B>
cgss::BlendFactor cgss::basic_RenderStatesData<B>::getBlendColorSrcFactor() const {
	return B::Ops::draw_state_get_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::ColorSrc);
}

template <class B>
cgss::BlendFactor cgss::basic_RenderStatesData<B>::getBlendColorDstFactor() const {
	return B::Ops::draw_state_get_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::ColorDst);
}

template <class B>
cgss::BlendFactor cgss::basic_RenderStatesData<B>::getBlendAlphaSrcFactor() const {
	return B::Ops::draw_state_get_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::AlphaSrc);
}

template <class B>
cgss::BlendFactor cgss::basic_RenderStatesData<B>::getBlendAlphaDstFactor() const {
	return B::Ops::draw_state_get_blend_factor(cgss::backend::native_of(m_drawState), cgss::backend::BlendFactorSlot::AlphaDst);
}

template <class B>
cgss::BlendEquation cgss::basic_RenderStatesData<B>::getBlendColorEquation() const {
	return B::Ops::draw_state_get_blend_equation(cgss::backend::native_of(m_drawState), cgss::backend::BlendEquationSlot::Color);
}

template <class B>
cgss::BlendEquation cgss::basic_RenderStatesData<B>::getBlendAlphaEquation() const {
	return B::Ops::draw_state_get_blend_equation(cgss::backend::native_of(m_drawState), cgss::backend::BlendEquationSlot::Alpha);
}

template <class B>
bool cgss::basic_RenderStatesData<B>::loadShaderFromMemory(const std::string& shader, cgss::ShaderType type) {
	CGSS_PROFILE_ZONE("cgss::RenderStatesData::loadShaderFromMemory");
	if (!isAvailable()) {
		warnIfShadersNotAvailable();
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<typename B::native_shader>();
	}
	const auto target = B::Ops::shader_preprocessor_target();
	const auto processed = preprocess<B>(shader, stageFromShaderType(type));
	bool success;
	if (target != Preprocessor::Target::DesktopFixedPipeline && type == cgss::ShaderType::Fragment) {
		// Without the fixed pipeline, the vertex stage must write the
		// varyings the processed fragment shader reads.
		success = B::Ops::shader_load_from_memory(*m_shader, Preprocessor::defaultVertexShader(target), processed.source);
	} else if (target == Preprocessor::Target::RaylibGlsl330 && type == cgss::ShaderType::Vertex) {
		// raylib's default fragment stage reads other varyings.
		success = B::Ops::shader_load_from_memory(*m_shader, processed.source, Preprocessor::defaultFragmentShader(target));
	} else {
		success = B::Ops::shader_load_from_memory(*m_shader, processed.source, type);
	}
	enableShader(success);
	if (!success) {
		std::cerr << "Shader content:" << std::endl << processed.source << std::endl;
		return false;
	}
	applyUniformDefaults<B>(*m_shader, processed.uniformDefaults);
	return true;
}

template <class B>
bool cgss::basic_RenderStatesData<B>::loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader) {
	CGSS_PROFILE_ZONE("cgss::RenderStatesData::loadShaderFromMemory");
	if (!isAvailable()) {
		warnIfShadersNotAvailable();
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<typename B::native_shader>();
	}
	const auto processedVert = preprocess<B>(vertexShader,   Preprocessor::Stage::Vertex);
	const auto processedFrag = preprocess<B>(fragmentShader, Preprocessor::Stage::Fragment);
	const bool success = B::Ops::shader_load_from_memory(*m_shader, processedVert.source, processedFrag.source);
	enableShader(success);
	if (!success) {
		std::cerr << "Vertex shader content:" << std::endl << processedVert.source << std::endl;
		std::cerr << "Fragment shader content:" << std::endl << processedFrag.source << std::endl;
		return false;
	}
	applyUniformDefaults<B>(*m_shader, concat(processedVert.uniformDefaults, processedFrag.uniformDefaults));
	return true;
}

template <class B>
bool cgss::basic_RenderStatesData<B>::loadShaderFromMemory(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader) {
	CGSS_PROFILE_ZONE("cgss::RenderStatesData::loadShaderFromMemory");
	if (B::Ops::shader_is_geometry_available() && isAvailable()) {
		if (m_shader == nullptr) {
			m_shader = std::make_unique<typename B::native_shader>();
		}
		const auto processedVert = preprocess<B>(vertexShader,   Preprocessor::Stage::Vertex);
		const auto processedGeom = preprocess<B>(geometryShader, Preprocessor::Stage::Geometry);
		const auto processedFrag = preprocess<B>(fragmentShader, Preprocessor::Stage::Fragment);
		const bool success = B::Ops::shader_load_from_memory(*m_shader, processedVert.source, processedGeom.source, processedFrag.source);
		enableShader(success);
		if (!success) {
			std::cerr << "Vertex shader content:" << std::endl << processedVert.source << std::endl;
			std::cerr << "Geometry shader content:" << std::endl << processedGeom.source << std::endl;
			std::cerr << "Fragment shader content:" << std::endl << processedFrag.source << std::endl;
			return false;
		}
		applyUniformDefaults<B>(*m_shader, concat(concat(processedVert.uniformDefaults, processedGeom.uniformDefaults),
		                                          processedFrag.uniformDefaults));
		return true;
	}
	// Shaders disabled / unavailable, or geometry shaders specifically
	// unsupported: surface the reason.
	if (!B::Ops::shader_is_geometry_available()) {
		std::cerr << "LiteCGSS: geometry-shader load attempted but the graphics backend "
		             "doesn't support geometry shaders."
		          << std::endl;
	} else {
		warnIfShadersNotAvailable();
	}
	enableShader(false);
	return false;
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, float x) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, x);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Vec2& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Vec3& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Vec4& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, int x) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, x);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec2& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec3& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec4& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, bool x) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, x);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec2& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec3& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec4& vector) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, vector);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Mat3& matrix) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, matrix);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const cgss::Glsl::Mat4& matrix) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, matrix);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, cgss::CurrentTextureType) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform_current_texture(*m_shader, name);
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniform(const std::string& name, const basic_Texture<B>& texture) {
	if (m_shader == nullptr) return;
	B::Ops::shader_set_uniform(*m_shader, name, cgss::backend::native_of(texture));
	// The preprocessor declares `<name>_factor_npot` for every secondary
	// sampler and scales its lookups by it: push the texture's factor too,
	// or it would be sampled at the padded size where storage is padded.
	const auto factor = texture.factorNpot();
	B::Ops::shader_set_uniform(*m_shader, name + "_factor_npot",
		cgss::Glsl::Vec2{ static_cast<float>(factor[0]), static_cast<float>(factor[1]) });
}

template <class B>
void cgss::basic_RenderStatesData<B>::setShaderUniformArray(const std::string& name, const float* scalarArray, std::size_t length) {
	if (m_shader != nullptr) B::Ops::shader_set_uniform_array(*m_shader, name, scalarArray, length);
}

template <class B>
bool cgss::basic_RenderStatesData<B>::enabledShader() const {
	return m_shadersEnabled;
}

template <class B>
const cgss::basic_DrawState<B>& cgss::basic_RenderStatesData<B>::getRenderStates() const {
	return m_drawState;
}

template class cgss::basic_RenderStatesData<cgss::backend::ActiveBackend>;
