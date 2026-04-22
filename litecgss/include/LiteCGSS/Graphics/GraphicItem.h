#ifndef CGSS_GRAPHIC_ITEM_H
#define CGSS_GRAPHIC_ITEM_H

#include "LiteCGSS/Graphics/RenderTarget.h"

namespace cgss {
	class GraphicItem {
	public:
		GraphicItem() = default;
		virtual ~GraphicItem() = default;

		virtual void draw(cgss::RenderTarget& target) const = 0;
		virtual void drawFast(cgss::RenderTarget& target) const = 0;
	};
}
#endif
