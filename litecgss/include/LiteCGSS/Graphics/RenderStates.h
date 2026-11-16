#ifndef CGSS_RENDER_STATES_H
#define CGSS_RENDER_STATES_H

#include <memory>
#include <string>

#include "LiteCGSS/Graphics/RenderStatesData.h"
#include "LiteCGSS/Common/BondElement.h"
#include "LiteCGSS/Common/Meta/metadata.h"

namespace cgss {

	class RenderStates :
		public BondElement<RenderStatesData> {
	private:
		void updateShader();
	public:
		RenderStates() = default;
		RenderStates(RenderStates&&) = default;
		RenderStates(const RenderStates&) = delete;
		RenderStates& operator=(const RenderStates&) = delete;
		RenderStates& operator=(RenderStates&&) = default;
		virtual ~RenderStates() = default;

		static bool areShadersEnabled();
		static bool areGeometryShadersEnabled();
		static void enableShaders(bool enable);

		// True iff the active GL context accepts the legacy fixed-pipeline
		// GLSL contract (gl_FragColor / gl_TexCoord[0] / texture2D / gl_Color).
		// Lets clients (e.g. PSDK) detect at runtime whether to author shaders
		// against the legacy or the modern pipeline. Cheap; safe to call before
		// the window opens.
		static bool supportsFixedPipeline();

		RenderStatesData& data() { return value(); }
	};

	namespace meta {
		template<>
		struct Log<RenderStates> {
			static constexpr auto classname = "RenderStates";
		};
	}

}

#endif