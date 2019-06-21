#ifndef LR_GRAPHICS_STACK_H
#define LR_GRAPHICS_STACK_H

#include "GraphicsContainer.h"

namespace lr {
	class GraphicsStackItem;
	class GraphicsStack {
	public:
		GraphicsStack() = default;
		virtual ~GraphicsStack();

		void add(GraphicsStackItem& el);
		void remove(GraphicsStackItem& el);
		void clear();
		void syncFromRawData(std::vector<GraphicsStackItem*> data, bool overrideOriginStack = true);
		std::size_t size() const { return m_stack.size(); }

	protected:
		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }

	private:
		GraphicsContainer m_stack;
	};
}
#endif