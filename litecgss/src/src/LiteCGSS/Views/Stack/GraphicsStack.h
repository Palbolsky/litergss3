#ifndef CGSS_GRAPHICS_STACK_H
#define CGSS_GRAPHICS_STACK_H

#include <mutex>
#include <ostream>
#include "GraphicsContainer.h"
#include "LiteCGSS/Graphics/GraphicsStackItemPtr.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"
#include "LiteCGSS/Graphics/ZIndex.h"

namespace cgss {

	template <class ZAlgorithmStrategy>
	class GraphicsStack {
	public:
		GraphicsStack() = default;

		GraphicsStack(const GraphicsStack&) = delete;
		GraphicsStack& operator=(const GraphicsStack&) = delete;
		GraphicsStack(GraphicsStack&&) = delete;
		GraphicsStack& operator=(GraphicsStack&&) = delete;

		virtual ~GraphicsStack() {
			clear();
		}

		static constexpr int32_t MaxZ() { return std::numeric_limits<int32_t>::max() >> 1; }
		static constexpr int32_t MinZ() { return 0; }

		std::size_t size() const { return m_stack.size(); }

		const auto& operator[](std::size_t index) const {
			return *m_stack[index];
		}

		auto& operator[](std::size_t index) {
			return *m_stack[index];
		}

		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }
		
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true) {
			if (forceDetachAll) {
				for(auto& it : m_stack) {
					it->m_originStack = nullptr;
				}
			}
			m_stack = std::move(data);
		}

		void clear() {
			syncFromRawData({});
		}

		void add(GraphicsStackItemPtr item) {
			item->m_originStack = this;
			item->m_priority.index = queryNextIndex();
			m_zSorter.add(std::move(item));
		}

		void set(ZIndex& item, long z) {
			m_zSorter.set(item, z);
		}

		void refreshZ() {
			m_zSorter.refresh();
		}

		bool remove(const GraphicsStackItem* item) {
			return m_zSorter.remove(item);
		}

	private:
		template <class ZAlgo>
		friend std::ostream& operator<<(std::ostream& os, const GraphicsStack<ZAlgo>& dt);

		std::int32_t queryNextIndex() {
			return m_zIndexCounter++;
		}

		GraphicsContainer m_stack;
		ZAlgorithmStrategy m_zSorter = ZAlgorithmStrategy { m_stack };

		int32_t m_zIndexCounter = 0;		
		mutable std::mutex m_mutexDetached;
		mutable GraphicsContainer m_detached;
		bool m_insertionZSort = false;
	};

	template<class ZAlgorithmStrategy>
	std::ostream& operator<<(std::ostream& os, const cgss::GraphicsStack<ZAlgorithmStrategy>& dt) {
		os << static_cast<const void*>(&dt) << "\t";
		for (const auto& element : dt) {
			os << "[" << element->getZ().z  << ", " << element->getZ().index << "]";
		}
		os << std::endl;
		return os;
	}

}

#endif