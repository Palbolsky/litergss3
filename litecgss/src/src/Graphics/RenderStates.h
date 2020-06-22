#ifndef CGSS_RENDER_STATES_H
#define CGSS_RENDER_STATES_H

#include <memory>
#include <string>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Shader.hpp>
#include "Common/Meta/metadata.h"

namespace cgss {

    class ShaderRenderable;
	class RenderStates {
	private:
		sf::RenderStates m_renderStates;
		std::unique_ptr<sf::Shader> m_shader;
		bool m_shadersEnabled = false;

		ShaderRenderable* m_target = nullptr;
		std::string m_lastError;
		
		void updateLinkedElement();
		void updateShader();
	public:
		RenderStates();
		RenderStates(RenderStates&&) = default;
		RenderStates(const RenderStates&) = delete;
		RenderStates& operator=(const RenderStates&) = delete;
		RenderStates& operator=(RenderStates&&) = default;
		virtual ~RenderStates();

		void enableShader(bool enable);
		void warnIfShadersNotAvailable();
		bool enabled() const;

		static bool areShadersEnabled();
		static bool areGeometryShadersEnabled();

		void setBlendColorSrcFactor(sf::BlendMode::Factor colorSrcFactor);
		void setBlendColorDstFactor(sf::BlendMode::Factor colorDstFactor);
		void setBlendAlphaSrcFactor(sf::BlendMode::Factor alphaSrcFactor);
		void setBlendAlphaDstFactor(sf::BlendMode::Factor alphaDstFactor);
		
		void setBlendColorEquation(sf::BlendMode::Equation colorEquation);
		void setBlendAlphaEquation(sf::BlendMode::Equation alphaEquation);

		sf::BlendMode::Factor getBlendColorSrcFactor() const;
		sf::BlendMode::Factor getBlendColorDstFactor() const;
		sf::BlendMode::Factor getBlendAlphaSrcFactor() const;
		sf::BlendMode::Factor getBlendAlphaDstFactor() const;
		
		sf::BlendMode::Equation getBlendColorEquation() const;
		sf::BlendMode::Equation getBlendAlphaEquation() const;

		bool loadShaderFromMemory(const std::string& shader, sf::Shader::Type type);
		bool loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader);
		bool loadShaderFromMemory(const std::string &vertexShader, const std::string& geometryShader, const std::string& fragmentShader);

		void setShaderUniform(const std::string& name, float x);
		void setShaderUniform(const std::string& name, const sf::Glsl::Vec2& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Vec3& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Vec4& vector);
		void setShaderUniform(const std::string& name, int x);
		void setShaderUniform(const std::string& name, const sf::Glsl::Ivec2& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Ivec3& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Ivec4& vector);
		void setShaderUniform(const std::string& name, bool x);
		void setShaderUniform(const std::string& name, const sf::Glsl::Bvec2& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Bvec3& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Bvec4& vector);
		void setShaderUniform(const std::string& name, const sf::Glsl::Mat3& matrix);
		void setShaderUniform(const std::string& name, const sf::Glsl::Mat4& matrix);
		void setShaderUniform(const std::string& name, const sf::Texture& texture);
		void setShaderUniform(const std::string& name, sf::Shader::CurrentTextureType);

		void setShaderUniformArray(const std::string& name, const float* scalarArray, std::size_t length);

		const sf::RenderStates& getRenderStates() const { return m_renderStates; }
		sf::RenderStates& getRenderStates() { return m_renderStates; }

		void bindElement(ShaderRenderable* target);
	};

	namespace meta {
		template<>
		struct Log<RenderStates> {
			static constexpr auto classname = "RenderStates";
		};
	}

}

#endif