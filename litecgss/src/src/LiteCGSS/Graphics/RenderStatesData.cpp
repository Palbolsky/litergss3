#include <sstream>
#include <fstream>
#include <iostream>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "RenderStatesData.h"
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

	m_drawState.raw().shader = m_shader.get();
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

void cgss::RenderStatesData::setBlendColorSrcFactor(cgss::BlendFactor colorSrcFactor) {
	m_drawState.raw().blendMode.colorSrcFactor = cgss::internal::toSf(colorSrcFactor);
}

void cgss::RenderStatesData::setBlendColorDstFactor(cgss::BlendFactor colorDstFactor) {
	m_drawState.raw().blendMode.colorDstFactor = cgss::internal::toSf(colorDstFactor);
}

void cgss::RenderStatesData::setBlendAlphaSrcFactor(cgss::BlendFactor alphaSrcFactor) {
	m_drawState.raw().blendMode.alphaSrcFactor = cgss::internal::toSf(alphaSrcFactor);
}

void cgss::RenderStatesData::setBlendAlphaDstFactor(cgss::BlendFactor alphaDstFactor) {
	m_drawState.raw().blendMode.alphaDstFactor = cgss::internal::toSf(alphaDstFactor);
}

void cgss::RenderStatesData::setBlendColorEquation(cgss::BlendEquation colorEquation) {
	m_drawState.raw().blendMode.colorEquation = cgss::internal::toSf(colorEquation);
}

void cgss::RenderStatesData::setBlendAlphaEquation(cgss::BlendEquation alphaEquation) {
	m_drawState.raw().blendMode.alphaEquation = cgss::internal::toSf(alphaEquation);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendColorSrcFactor() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.colorSrcFactor);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendColorDstFactor() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.colorDstFactor);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendAlphaSrcFactor() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.alphaSrcFactor);
}

cgss::BlendFactor cgss::RenderStatesData::getBlendAlphaDstFactor() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.alphaDstFactor);
}

cgss::BlendEquation cgss::RenderStatesData::getBlendColorEquation() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.colorEquation);
}

cgss::BlendEquation cgss::RenderStatesData::getBlendAlphaEquation() const {
	return cgss::internal::fromSf(m_drawState.raw().blendMode.alphaEquation);
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
		m_shader->setUniform(name, texture.raw());
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
