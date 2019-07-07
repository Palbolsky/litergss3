#ifndef LR_GRAPHICS_STACK_ITEM_H
#define LR_GRAPHICS_STACK_ITEM_H

#include <SFML/Graphics.hpp>
#include "GraphicsContainer.h"

namespace lr {
	class GraphicsStack;
	class GraphicsStackItem {
	private:
		//Access to overrideOriginStack
		friend class GraphicsStack;

		void setOriginStack(GraphicsContainer *o);

		GraphicsContainer* m_originStack = nullptr;
		unsigned long m_drawPriority = 0;

	protected:
		GraphicsStackItem() = default;
		void setOriginStack(GraphicsContainer& o) { setOriginStack(&o); }
		void resetOriginStack() { setOriginStack(nullptr); }
		void overrideOriginStack(GraphicsContainer* stack = nullptr) { m_originStack = stack; }

	public:
		virtual ~GraphicsStackItem() { resetOriginStack(); }
		unsigned long getDrawPriority() const { return m_drawPriority; }

		virtual void draw(sf::RenderTarget& target) const = 0;
		virtual void drawFast(sf::RenderTarget& target) const = 0;
	};
}
#endif