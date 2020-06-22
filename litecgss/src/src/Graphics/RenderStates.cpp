#include <SFML/Graphics/BlendMode.hpp>
#include "ShaderRenderable.h"
#include "RenderStates.h"

cgss::RenderStates::RenderStates() :
    m_shadersEnabled(sf::Shader::isAvailable()) {
    updateShader();
}

void cgss::RenderStates::warnIfShadersNotAvailable() {
    if (!sf::Shader::isAvailable()) {
        m_lastError = "Shaders are not available :(";
    }
}

void cgss::RenderStates::updateShader() {
    if (m_shadersEnabled && m_shader == nullptr) {
        m_shader = std::make_unique<sf::Shader>();
        m_renderStates.shader = m_shader.get();
    } else if (!m_shadersEnabled && m_shader != nullptr) {
        m_renderStates.shader = nullptr;
        m_shader = nullptr;
    }
}

void cgss::RenderStates::enableShader(bool enable) {
    if (enable) { 
        warnIfShadersNotAvailable();
    }
    m_shadersEnabled = enable && sf::Shader::isAvailable();
    updateShader();
}

bool cgss::RenderStates::areGeometryShadersEnabled() {
    return sf::Shader::isGeometryAvailable();
}

bool cgss::RenderStates::areShadersEnabled() {
    return sf::Shader::isAvailable();
}

void cgss::RenderStates::setBlendColorSrcFactor(sf::BlendMode::Factor colorSrcFactor) {
    m_renderStates.blendMode.colorSrcFactor = std::move(colorSrcFactor);
}

void cgss::RenderStates::setBlendColorDstFactor(sf::BlendMode::Factor colorDstFactor) {
    m_renderStates.blendMode.colorDstFactor = std::move(colorDstFactor);
}

void cgss::RenderStates::setBlendAlphaSrcFactor(sf::BlendMode::Factor alphaSrcFactor) {
    m_renderStates.blendMode.alphaSrcFactor = std::move(alphaSrcFactor);
}

void cgss::RenderStates::setBlendAlphaDstFactor(sf::BlendMode::Factor alphaDstFactor) {
    m_renderStates.blendMode.alphaDstFactor = std::move(alphaDstFactor);
}

void cgss::RenderStates::setBlendColorEquation(sf::BlendMode::Equation colorEquation) {
    m_renderStates.blendMode.colorEquation = colorEquation;
}

void cgss::RenderStates::setBlendAlphaEquation(sf::BlendMode::Equation alphaEquation) {
    m_renderStates.blendMode.alphaEquation = alphaEquation;
}

sf::BlendMode::Factor cgss::RenderStates::getBlendColorSrcFactor() const {
    return m_renderStates.blendMode.colorSrcFactor;
}

sf::BlendMode::Factor cgss::RenderStates::getBlendColorDstFactor() const {
    return m_renderStates.blendMode.colorDstFactor;
}

sf::BlendMode::Factor cgss::RenderStates::getBlendAlphaSrcFactor() const {
    return m_renderStates.blendMode.alphaSrcFactor;
}

sf::BlendMode::Factor cgss::RenderStates::getBlendAlphaDstFactor() const {
    return m_renderStates.blendMode.alphaDstFactor;
}

sf::BlendMode::Equation cgss::RenderStates::getBlendColorEquation() const {
    return m_renderStates.blendMode.colorEquation;
}

sf::BlendMode::Equation cgss::RenderStates::getBlendAlphaEquation() const {
    return m_renderStates.blendMode.alphaEquation;
}

bool cgss::RenderStates::loadShaderFromMemory(const std::string &shader, sf::Shader::Type type) {
    if (m_shader != nullptr) {
        return m_shader->loadFromMemory(shader, type);
    }
    return false;
}

bool cgss::RenderStates::loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader) {
    if (m_shader != nullptr) {
        return m_shader->loadFromMemory(vertexShader, fragmentShader);
    }
    return false;
}

bool cgss::RenderStates::loadShaderFromMemory(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader) {
    if (m_shader != nullptr && sf::Shader::isGeometryAvailable()) {
        return m_shader->loadFromMemory(vertexShader, geometryShader, fragmentShader);
    }
    return false;
}

void cgss::RenderStates::setShaderUniform(const std::string& name, float x) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, x);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Vec2& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Vec3& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Vec4& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, int x) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, x);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Ivec2& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Ivec3& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Ivec4& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, bool x) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, x);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Bvec2& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Bvec3& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Bvec4& vector) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, vector);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Mat3& matrix) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, matrix);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Glsl::Mat4& matrix) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, matrix);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, sf::Shader::CurrentTextureType x) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, x);
    }
}

void cgss::RenderStates::setShaderUniform(const std::string& name, const sf::Texture& texture) {
    if (m_shader != nullptr) {
        m_shader->setUniform(name, texture);
    }
}

void cgss::RenderStates::setShaderUniformArray(const std::string& name, const float* scalarArray, std::size_t length) {
    if (m_shader != nullptr) {
        m_shader->setUniformArray(name, scalarArray, length);
    }
}

bool cgss::RenderStates::enabled() const {
    return m_shadersEnabled;
}

void cgss::RenderStates::bindElement(ShaderRenderable* target) {
    if (target != m_target) {
        auto lastLinkedElement = m_target;
        m_target = nullptr;
        if (lastLinkedElement != nullptr) {
            lastLinkedElement->bindRenderStates(nullptr);
        }
        m_target = target;
        if (m_target != nullptr) {
            m_target->bindRenderStates(this);
        }
    }
}

cgss::RenderStates::~RenderStates() {
    bindElement(nullptr);
}
