#ifndef CGSS_GRAPHICS_STACK_H
#define CGSS_GRAPHICS_STACK_H

#include <mutex>
#include "GraphicsContainer.h"
#include "Graphics/GraphicsStackItemPtr.h"
#include "Graphics/ZIndex.h"

namespace cgss {
	class GraphicsStack {
		// friend because it has to be able to manipulate z index swaps and auto detaching system
		friend class GraphicsStackItem;
	public:
		GraphicsStack() = default;
		virtual ~GraphicsStack();

		static constexpr auto MaxZ() { return std::numeric_limits<int32_t>::max(); }
		static constexpr int32_t MinZ() { return 0; }

		void add(GraphicsStackItemPtr item);
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true);
		std::size_t size() const { return m_stack.size(); }
		ZIndex currentZIndex() const { return { MaxZ(), m_zIndexCounter }; }

		const auto& operator[](std::size_t index) const {
			return *m_stack[index];
		}

		auto& operator[](std::size_t index) {
			return *m_stack[index];
		}

		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }
		void clear();
		
	private:
		bool remove(GraphicsStackItem* item);
		void zSwap(const ZIndex& previous, const ZIndex& next);

		void leftCircleShift(std::size_t begin, std::size_t end);
		void rightCircleShift(std::size_t begin, std::size_t end);
		std::size_t dichotomicFindFirstGreater(const ZIndex& value) const;

		GraphicsContainer m_stack;
		int32_t m_zIndexCounter = 0;
		mutable std::mutex m_mutexDetached;
		mutable GraphicsContainer m_detached;
	};
}
#endif