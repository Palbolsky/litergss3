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
		bool m_visible = true;

	public:
		GraphicsStackItem() = default;
		virtual ~GraphicsStackItem();
		
		void detach();

		unsigned long getDrawPriority() const { return m_drawPriority; }

		void tryDraw(sf::RenderTarget& target) const { if (m_visible) { draw(target); } }
		void tryDrawFast(sf::RenderTarget& target) const { if (m_visible) { drawFast(target); } }

		void setVisible(bool value) { m_visible = value; };
		bool isVisible() const { return m_visible; };

	protected:
		virtual void draw(sf::RenderTarget& target) const = 0;
		virtual void drawFast(sf::RenderTarget& target) const = 0;
	};

}
#endif