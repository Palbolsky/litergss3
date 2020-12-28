#ifndef CGSS_Z_BASE_CONTAINER_H
#define CGSS_Z_BASE_CONTAINER_H

#include <vector>
#include "LoggerStack.h"
#include "ZGetter.h"
#include "LiteCGSS/Graphics/ZIndex.h"

namespace cgss {
	struct ZContainerLog;
}
SKA_LOGC_CONFIG(ska::LogLevel::Error, cgss::ZContainerLog);

namespace cgss {

	template <class ZIndexedElement>
	class ZBaseContainer {
	protected:
		using ZGet = ZGetter<ZIndexedElement>;
		std::vector<ZIndexedElement> m_stack;

	public:
		ZBaseContainer() = default;
		virtual ~ZBaseContainer() = default;

		std::size_t size() const { return m_stack.size(); }

		const auto& operator[](std::size_t index) const {
			return *m_stack[index];
		}

		auto& operator[](std::size_t index) {
			return *m_stack[index];
		}

		auto begin() const { return m_stack.begin(); }
		auto end() const { return m_stack.end(); }

		std::size_t findFirstGreater(const ZIndex& value) const {
			const auto index = dichotomicFindFirstGreater(value);
			if (m_stack.empty()) {
				return 0;
			}

			if (index == 1) {
				const auto diff = (value - ZGet::z(m_stack[index])).sign();
				SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Debug, cgss::ZContainerLog) << "\tAdapting index " << index << " with diff = " << diff << " for value z = " << value.z << ", " << value.index;
				if (diff == -1) {
					return (value - ZGet::z(m_stack[0])).sign() <= 0 ? 0 : 1;
				}
				return diff + index;
			}

			const auto diff = (value - ZGet::z(m_stack[index])).sign();
			const auto finalIndex = index + (diff <= 0 ? 0 : 1);
			SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Debug, cgss::ZContainerLog) << "\tAdapting index " << index << " to index = " << finalIndex << " for value z = " << value.z << ", " << value.index;
			return finalIndex;
		}

		template<class ZPredicate>
		bool remove(const ZIndex& itemZ, ZPredicate predicate) {
			const auto index = findFirstGreater(itemZ);

			const auto foundExactValue = predicate(m_stack[index]);
			if (!foundExactValue) {
				SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Error, cgss::ZContainerLog) << static_cast<void*>(this) << " Trying to remove an element from stack but unable to find it for z-index = " << itemZ.z << ", " << itemZ.index
					<< " only found position = " << index << " but no item equality";
				//SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Error, cgss::ZContainerLog) << *this;
				return false;
			}

			SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Info, cgss::ZContainerLog) << "Element removed from stack at index " << index << " for z-index = " << itemZ.z << ", " << itemZ.index;

			m_stack.erase(m_stack.begin() + index);
			return true;
		}

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
}

#endif