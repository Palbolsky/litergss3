#ifndef CGSS_GRAPHICS_STACK_H
#define CGSS_GRAPHICS_STACK_H

#include <mutex>
#include "GraphicsContainer.h"
#include "Graphics/GraphicsStackItemPtr.h"

namespace cgss {
	class GraphicsStack {
	public:
		GraphicsStack() = default;
		virtual ~GraphicsStack();

		void add(GraphicsStackItemPtr item);
		bool transferToDetached(GraphicsStackItem* item);
		
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true);
		std::size_t getSize() const { return m_stack.size(); }

	protected:
		void freeDetached() const;
		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }

	private:
		void clear();

		GraphicsContainer m_stack;

		mutable std::mutex m_mutexDetached;
		mutable GraphicsContainer m_detached;
	};
}
#endif