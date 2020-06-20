#ifndef CGSS_GRAPHICS_STACK_ITEM_H
#define CGSS_GRAPHICS_STACK_ITEM_H
#include <memory>
#include "GraphicItem.h"
#include "Views/Stack/GraphicsStack.h"
#include "ZIndex.h"

namespace cgss {
	class GraphicsStack;
	class Rectangle;

	template <class Item>
	class GraphicsStackBinder {
		//"sudo forceAttach / forceDetach" :)
		friend class GraphicsStack;
	private:
		void forceDetach() { 
			auto& ref = static_cast<Item&>(*this);
			ref.m_originStack = nullptr;
			ref.m_priority = ZIndex{};
		}
		
		void forceAttach(GraphicsStack& originStack) {
			auto& ref = static_cast<Item&>(*this);
			ref.m_originStack = &originStack;
			ref.m_priority.index = originStack.queryNextIndex();
		}
	};

	class GraphicsStackItem : 
		public GraphicsStackBinder<GraphicsStackItem>,
		protected GraphicItem {
		friend class GraphicsStackBinder<GraphicsStackItem>;
	private:
		GraphicsStack* m_originStack = nullptr;
		Rectangle* m_boundRectangle = nullptr;
		ZIndex m_priority;
		bool m_visible = true;

	public:
		GraphicsStackItem() = default;
		GraphicsStackItem(const GraphicsStackItem&) = delete;
		GraphicsStackItem& operator=(const GraphicsStackItem&) = delete;
		GraphicsStackItem(GraphicsStackItem&&) = delete;
		GraphicsStackItem& operator=(GraphicsStackItem&&) = delete;
		virtual ~GraphicsStackItem();
		virtual void updateFromRectangle(const sf::IntRect&) {};
		virtual void setOpacity(std::uint8_t) {};

		void detach();

		const ZIndex& getZ() const { return m_priority; }
		void setZ(long z);

		void bindRectangle(Rectangle* rectangle);
		Rectangle* getBoundRectangle() { return m_boundRectangle; }
		const Rectangle* getBoundRectangle() const { return m_boundRectangle; }

		void tryDraw(sf::RenderTarget& target) const;
		void tryDrawFast(sf::RenderTarget& target) const;

		void setVisible(bool value) { m_visible = value; };
		bool isVisible() const { return m_visible; };
	};

}
#endif