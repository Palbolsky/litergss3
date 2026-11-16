#ifndef CGSS_SHADER_RENDERABLE_H
#define CGSS_SHADER_RENDERABLE_H

#include "LiteCGSS/Graphics/RenderStatesData.h"
#include "LiteCGSS/Common/Bindable.h"

namespace cgss {
	using ShaderRenderable = MultiBindable<RenderStatesData>;
}

#endif
