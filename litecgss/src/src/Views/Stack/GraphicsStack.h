#ifndef CGSS_GRAPHICS_STACK_H
#define CGSS_GRAPHICS_STACK_H

#include <mutex>
#include "GraphicsContainer.h"
#include "Graphics/GraphicsStackItemPtr.h"
#include "Graphics/ZIndex.h"

namespace cgss {
	class GraphicsStack {
	public:
		GraphicsStack() = default;
		virtual ~GraphicsStack();

		void add(GraphicsStackItemPtr item);
		bool transferToDetached(GraphicsStackItem* item);
		void zSwap(const ZIndex& previous, const ZIndex& next); 
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true);
		std::size_t getSize() const { return m_stack.size(); }
		ZIndex currentZIndex() const { return {std::numeric_limits<long>::max(), m_zIndexCounter}; }

	protected:
		void freeDetached() const;
		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }

	private:
		void leftCircleShift(std::size_t begin, std::size_t end);
		void rightCircleShift(std::size_t begin, std::size_t end);
		std::size_t dichotomicFindFirstGreater(const ZIndex& value) const;
		void clear();

		GraphicsContainer m_stack;
		long m_zIndexCounter = 0;
		mutable std::mutex m_mutexDetached;
		mutable GraphicsContainer m_detached;
	};
}
#endif