#include <sstream>
#include <fstream>
#include <iostream>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "RenderStatesData.h"
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Internal/SfShaderConv.h"
#include "LiteCGSS/Graphics/Texture.h"

bool cgss::RenderStatesData::SHADERS_ENABLED = true;

cgss::RenderStatesData::RenderStatesData() :
	m_shadersEnabled(true) {
}

cgss::RenderStatesData::RenderStatesData(RenderStatesData&&) = default;
cgss::RenderStatesData& cgss::RenderStatesData::operator=(RenderStatesData&&) = default;
cgss::RenderStatesData::~RenderStatesData() = default;

void cgss::RenderStatesData::enableShaders(bool enable) {
	SHADERS_ENABLED = enable;
}

bool cgss::RenderStatesData::enabledShaders() {
	return SHADERS_ENABLED;
}

void cgss::RenderStatesData::warnIfShadersNotAvailable() {
	if (SHADERS_ENABLED && !sf::Shader::isAvailable()) {
		std::string m_lastError = "Shaders are not available :(";
		//TODO
	}
}

void cgss::RenderStatesData::updateShader() {
	if (!m_shadersEnabled) {
		m_shader = nullptr;
	}

	cgss::backend::ActiveBackend::Ops::draw_state_set_shader(
		cgss::backend::native_of(m_drawState), m_shader.get());
}

bool cgss::RenderStatesData::isAvailable() {
	return SHADERS_ENABLED && sf::Shader::isAvailable();
}

void cgss::RenderStatesData::enableShader(bool enable) {
	if (enable) {
		warnIfShadersNotAvailable();
	}
	m_shadersEnabled = enable && isAvailable();
	updateShader();
}

namespace { using Ops = cgss::backend::ActiveBackend::Ops; }
using cgss::backend::native_of;
using cgss::backend::BlendFactorSlot;
using cgss::backend::BlendEquationSlot;

void cgss::RenderStatesData::setBlendColorSrcFactor(cgss::BlendFactor colorSrcFactor) {
	Ops::draw_state_set_blend_factor(native_of(m_drawState), BlendFactorSlot::ColorSrc, colorSrcFactor);
}

void cgss::RenderStatesData::setBlendColorDstFactor(cgss::BlendFactor colorDstFactor) {
	Ops::draw_state_set_blend_factor(native_of(m_drawState), BlendFactorSlot::ColorDst, colorDstFactor);
}

void cgss::RenderStatesData::setBlendAlphaSrcFactor(cgss::BlendFactor alphaSrcFactor) {
	Ops::draw_state_set_blend_factor(native_of(m_drawState), BlendFactorSlot::AlphaSrc, alphaSrcFactor);
}

void cgss::RenderStatesData::setBlendAlphaDstFactor(cgss::BlendFactor alphaDstFactor) {
	Ops::draw_state_set_blend_factor(native_of(m_drawState), BlendFactorSlot::AlphaDst, alphaDstFactor);
}

void cgss::RenderStatesData::setBlendColorEquation(cgss::BlendEquation colorEquation) {
	Ops::draw_state_set_blend_equation(native_of(m_drawState), BlendEquationSlot::Color, colorEquation);
}

void cgss::RenderStatesData::setBlendAlphaEquation(cgss::BlendEquation alphaEquation) {
	Ops::draw_state_set_blend_equation(native_of(m_drawState), BlendEquationSlot::Alpha, alphaEquation);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendColorSrcFactor() const {
	return Ops::draw_state_get_blend_factor(native_of(m_drawState), BlendFactorSlot::ColorSrc);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendColorDstFactor() const {
	return Ops::draw_state_get_blend_factor(native_of(m_drawState), BlendFactorSlot::ColorDst);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendAlphaSrcFactor() const {
	return Ops::draw_state_get_blend_factor(native_of(m_drawState), BlendFactorSlot::AlphaSrc);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendAlphaDstFactor() const {
	return Ops::draw_state_get_blend_factor(native_of(m_drawState), BlendFactorSlot::AlphaDst);
}

cgss::BlendEquation cgss::RenderStatesData::getBlendColorEquation() const {
	return Ops::draw_state_get_blend_equation(native_of(m_drawState), BlendEquationSlot::Color);
}

cgss::BlendEquation cgss::RenderStatesData::getBlendAlphaEquation() const {
	return Ops::draw_state_get_blend_equation(native_of(m_drawState), BlendEquationSlot::Alpha);
}

bool cgss::RenderStatesData::loadShaderFromMemory(const std::string &shader, cgss::ShaderType type) {
	if (!isAvailable()) {
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<sf::Shader>();
	}
	const bool success = m_shader->loadFromMemory(shader, cgss::internal::toSf(type));
	enableShader(success);
	if (!success) {
		std::cerr << "Shader content:" << std::endl << shader << std::endl;
		return false;
	}
	return true;
}

bool cgss::RenderStatesData::loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader) {
	if (!isAvailable()) {
		return false;
	}
	if (m_shader == nullptr) {
		m_shader = std::make_unique<sf::Shader>();
	}
	const bool success = m_shader->loadFromMemory(vertexShader, fragmentShader);
	enableShader(success);
	if (!success) {
		std::cerr << "Vertex shader content:" << std::endl << vertexShader << std::endl;
		std::cerr << "Fragment shader content:" << std::endl << fragmentShader << std::endl;
		return false;
	}
	return true;
}

bool cgss::RenderStatesData::loadShaderFromMemory(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader) {
	if (sf::Shader::isGeometryAvailable() && isAvailable()) {
		if (m_shader == nullptr) {
			m_shader = std::make_unique<sf::Shader>();
		}
		const bool success = m_shader->loadFromMemory(vertexShader, geometryShader, fragmentShader);
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

void cgss::RenderStatesData::setShaderUniform(const std::string& name, float x) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, x);
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Vec2& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Vec3& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Vec4& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, int x) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, x);
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec2& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec3& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Ivec4& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, bool x) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, x);
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec2& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec3& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Bvec4& vector) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(vector));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Mat3& matrix) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(matrix));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Glsl::Mat4& matrix) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::internal::toSf(matrix));
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, cgss::CurrentTextureType) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, sf::Shader::CurrentTexture);
	}
}

void cgss::RenderStatesData::setShaderUniform(const std::string& name, const cgss::Texture& texture) {
	if (m_shader != nullptr) {
		m_shader->setUniform(name, cgss::backend::native_of(texture));
	}
}

void cgss::RenderStatesData::setShaderUniformArray(const std::string& name, const float* scalarArray, std::size_t length) {
	if (m_shader != nullptr) {
		m_shader->setUniformArray(name, scalarArray, length);
	}
}

bool cgss::RenderStatesData::enabledShader() const {
	return m_shadersEnabled;
}

const cgss::DrawState& cgss::RenderStatesData::getRenderStates() const {
	return m_drawState;
}
