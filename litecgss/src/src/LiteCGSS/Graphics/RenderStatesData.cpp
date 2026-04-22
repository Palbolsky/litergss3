#include <iostream>

#include "RenderStatesData.h"
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Texture.h"

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
	if (SHADERS_ENABLED && !B::Ops::shader_is_available()) {
		std::string m_lastError = "Shaders are not available :(";
		//TODO
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
	if (enable) {
		warnIfShadersNotAvailable();
	}
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
	if (!isAvailable()) {
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<typename B::native_shader>();
	}
	const bool success = B::Ops::shader_load_from_memory(*m_shader, shader, type);
	enableShader(success);
	if (!success) {
		std::cerr << "Shader content:" << std::endl << shader << std::endl;
		return false;
	}
	return true;
}

template <class B>
bool cgss::basic_RenderStatesData<B>::loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader) {
	if (!isAvailable()) {
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<typename B::native_shader>();
	}
	const bool success = B::Ops::shader_load_from_memory(*m_shader, vertexShader, fragmentShader);
	enableShader(success);
	if (!success) {
		std::cerr << "Vertex shader content:" << std::endl << vertexShader << std::endl;
		std::cerr << "Fragment shader content:" << std::endl << fragmentShader << std::endl;
		return false;
	}
	return true;
}

template <class B>
bool cgss::basic_RenderStatesData<B>::loadShaderFromMemory(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader) {
	if (B::Ops::shader_is_geometry_available() && isAvailable()) {
		if (m_shader == nullptr) {
			m_shader = std::make_unique<typename B::native_shader>();
		}
		const bool success = B::Ops::shader_load_from_memory(*m_shader, vertexShader, geometryShader, fragmentShader);
		enableShader(success);
		if (!success) {
			std::cerr << "Vertex shader content:" << std::endl << vertexShader << std::endl;
			std::cerr << "Geometry shader content:" << std::endl << geometryShader << std::endl;
			std::cerr << "Fragment shader content:" << std::endl << fragmentShader << std::endl;
			return false;
		}
		return true;
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
	if (m_shader != nullptr) B::Ops::shader_set_uniform(*m_shader, name, cgss::backend::native_of(texture));
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
