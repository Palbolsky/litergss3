#ifndef CGSS_DRAWABLE_STACK_H
#define CGSS_DRAWABLE_STACK_H

#include <cstdint>
#include "LiteCGSS/Graphics/RenderTarget.h"
#include "LiteCGSS/Graphics/Camera.h"
#include "LiteCGSS/Views/Stack/GraphicsStack.h"
#include "LiteCGSS/Views/Stack/MainGraphicsStack.h"

namespace cgss {
	class DrawableStack :
		public MainGraphicsStack {
	public:
		using MainGraphicsStack::MainGraphicsStack;
		void draw(cgss::Camera& defview, cgss::RenderTarget& target) const;
		void drawFast(cgss::RenderTarget& target) const;
		void updateContentsOpacity(std::uint8_t opacity);
	};
}
#endif
