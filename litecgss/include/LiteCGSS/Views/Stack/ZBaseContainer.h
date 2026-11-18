#ifndef CGSS_Z_BASE_CONTAINER_H
#define CGSS_Z_BASE_CONTAINER_H

#include <cstddef>
#include <ostream>
#include <vector>
#include "LiteCGSS/Views/Stack/ZGetter.h"
#include "LiteCGSS/Graphics/ZIndex.h"

namespace cgss {
	namespace detail {
		// Logged by src/Views/Stack/ZContainerLog.cpp, out of line:
		// logging stays out of the public headers
		void LogZRemoveMiss(const void* container, const ZIndex& itemZ, std::size_t position);
	}

	template <class ZIndexedElement>
	class ZBaseContainer {
	protected:
		using ZGet = ZGetter<ZIndexedElement>;
		std::vector<ZIndexedElement> m_stack;

	public:
		ZBaseContainer() = default;
		ZBaseContainer(std::vector<ZIndexedElement> stack) :
			m_stack(std::move(stack)) {
		}
		virtual ~ZBaseContainer() = default;

		std::size_t size() const { return m_stack.size(); }

		const auto& operator[](std::size_t index) const {
			return m_stack[index];
		}

		auto& operator[](std::size_t index) {
			return m_stack[index];
		}

		bool operator==(const std::vector<ZIndexedElement>& vec) const {
			return m_stack == vec;
		}

		bool operator!=(const std::vector<ZIndexedElement>& vec) const {
			return m_stack != vec;
		}

		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }

		std::size_t findFirstGreater(const ZIndex& value) const {
			const auto index = dichotomicFindFirstGreater(value);
			if (m_stack.empty()) {
				return 0;
			}

			const auto diff = (value - ZGet::z(m_stack[index])).sign();
			if (index == 1) {
				if (diff == -1) {
					return (value - ZGet::z(m_stack[0])).sign() <= 0 ? 0 : 1;
				}
				return diff + index;
			}

			const auto finalIndex = index + (diff <= 0 ? 0 : 1);
			return finalIndex;
		}

		template<class ZPredicate>
		bool removeOne(const ZIndex& itemZ, ZPredicate predicate) {
			const auto index = findFirstGreater(itemZ);

			const auto foundExactValue = predicate(m_stack[index]);
			if (!foundExactValue) {
				detail::LogZRemoveMiss(this, itemZ, index);
				return false;
			}

			m_stack.erase(m_stack.begin() + index);
			return true;
		}

		template<class ZIndexedEl>
		friend std::ostream& operator<<(std::ostream& os, const ZBaseContainer<ZIndexedEl>& dt);

	private:
		std::size_t dichotomicFindFirstGreater(const ZIndex& value) const {
			if (m_stack.empty()) {
				return 0;
			}

			std::size_t min = 0;
			std::size_t max = m_stack.size() - 1;
			while (max - min > 1) {
				const auto index = (max + min) >> 1;
				const auto diff = (value - ZGet::z(m_stack[index])).sign();
				switch (diff) {
					case 0:
					return index;
					case 1:
					min = index;
					break;
					case -1:
					max = index;
					break;
				}
			}

			return max;
		}

	};

	template<class ZIndexedElement>
	std::ostream& operator<<(std::ostream& os, const ZBaseContainer<ZIndexedElement>& stack) {
		os << static_cast<const void*>(&stack) << "\t";
		for (const auto& element : stack.m_stack) {
			const auto elementZ = ZGetter<ZIndexedElement>::z(element);
			os << "[" << elementZ.z  << ", " << elementZ.index << "]";
		}
		return os;
	}

}

#endif