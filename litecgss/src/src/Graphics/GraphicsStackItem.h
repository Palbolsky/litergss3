#ifndef CGSS_GRAPHICS_STACK_ITEM_H
#define CGSS_GRAPHICS_STACK_ITEM_H
#include <memory>
#include <SFML/Graphics.hpp>

namespace cgss {
	class GraphicsStack;
	class Rectangle;

	template <class Item>
	class GraphicsStackBinder {
		//"sudo forceAttach / forceDetach" :)
		friend class GraphicsStack;
	private:
		void forceDetach() { static_cast<Item&>(*this).m_originStack = nullptr; }
		void forceAttach(GraphicsStack& originStack) { static_cast<Item&>(*this).m_originStack = &originStack; }
	};

	class GraphicsStackItem : 
		public GraphicsStackBinder<GraphicsStackItem> {
		friend class GraphicsStackBinder<GraphicsStackItem>;
	private:
		GraphicsStack* m_originStack = nullptr;
		unsigned long m_drawPriority = 0;
		Rectangle* m_rect = nullptr;
					
	public:
		GraphicsStackItem() = default;
		virtual ~GraphicsStackItem();
		
		void detach();

		void bindRectangle(Rectangle* rect);
		Rectangle* getBoundRectangle() const { return m_rect; };

		unsigned long getDrawPriority() const { return m_drawPriority; }

		virtual void draw(sf::RenderTarget& target) const = 0;
		virtual void drawFast(sf::RenderTarget& target) const = 0;
	};

}
#endif