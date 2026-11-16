#ifndef CGSS_RENDER_STATES_DATA_H
#define CGSS_RENDER_STATES_DATA_H

#include <memory>
#include <string>
#include <cstddef>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/BlendMode.h"
#include "LiteCGSS/Graphics/DrawState.h"
#include "LiteCGSS/Graphics/Glsl.h"
#include "LiteCGSS/Graphics/ShaderType.h"
#include "LiteCGSS/Graphics/TextureFwd.h"

namespace cgss {

	template <class B>
	class basic_RenderStatesData {
	private:
		basic_DrawState<B> m_drawState {};
		std::unique_ptr<typename B::native_shader> m_shader {};
		bool m_shadersEnabled = false;

		static bool SHADERS_ENABLED;

		void updateShader();
		static bool isAvailable();
	public:
		basic_RenderStatesData();
		basic_RenderStatesData(basic_RenderStatesData&&);
		basic_RenderStatesData(const basic_RenderStatesData&) = delete;
		basic_RenderStatesData& operator=(const basic_RenderStatesData&) = delete;
		basic_RenderStatesData& operator=(basic_RenderStatesData&&);
		virtual ~basic_RenderStatesData();

		static void enableShaders(bool enable);
		static bool enabledShaders();

		void enableShader(bool enable);
		void warnIfShadersNotAvailable();
		bool enabledShader() const;

		void setBlendColorSrcFactor(cgss::BlendFactor colorSrcFactor);
		void setBlendColorDstFactor(cgss::BlendFactor colorDstFactor);
		void setBlendAlphaSrcFactor(cgss::BlendFactor alphaSrcFactor);
		void setBlendAlphaDstFactor(cgss::BlendFactor alphaDstFactor);

		void setBlendColorEquation(cgss::BlendEquation colorEquation);
		void setBlendAlphaEquation(cgss::BlendEquation alphaEquation);

		cgss::BlendFactor getBlendColorSrcFactor() const;
		cgss::BlendFactor getBlendColorDstFactor() const;
		cgss::BlendFactor getBlendAlphaSrcFactor() const;
		cgss::BlendFactor getBlendAlphaDstFactor() const;

		cgss::BlendEquation getBlendColorEquation() const;
		cgss::BlendEquation getBlendAlphaEquation() const;

		bool loadShaderFromMemory(const std::string& shader, cgss::ShaderType type);
		bool loadShaderFromMemory(const std::string& vertexShader, const std::string& fragmentShader);
		bool loadShaderFromMemory(const std::string &vertexShader, const std::string& geometryShader, const std::string& fragmentShader);

		void setShaderUniform(const std::string& name, float x);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Vec2& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Vec3& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Vec4& vector);
		void setShaderUniform(const std::string& name, int x);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Ivec2& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Ivec3& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Ivec4& vector);
		void setShaderUniform(const std::string& name, bool x);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Bvec2& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Bvec3& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Bvec4& vector);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Mat3& matrix);
		void setShaderUniform(const std::string& name, const cgss::Glsl::Mat4& matrix);
		void setShaderUniform(const std::string& name, const basic_Texture<B>& texture);
		void setShaderUniform(const std::string& name, cgss::CurrentTextureType);

		void setShaderUniformArray(const std::string& name, const float* scalarArray, std::size_t length);

		const basic_DrawState<B>& getRenderStates() const;
	};

	using RenderStatesData = basic_RenderStatesData<cgss::backend::ActiveBackend>;
}

#endif
