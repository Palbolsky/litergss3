#include "LoggerStack.h"
#include "ZAlgorithm.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::ZAlgorithm);

#define LOG_DEBUG SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Debug, cgss::ZAlgorithm)
#define LOG_INFO SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Info, cgss::ZAlgorithm)
#define LOG_ERROR SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Error, cgss::ZAlgorithm)

std::size_t cgss::ZAlgorithm::dichotomicFindFirstGreater(const ZIndex& value) const {
	if (m_stack.empty()) {
		return 0;
	}

	std::size_t min = 0;
	std::size_t max = m_stack.size() - 1;
	while (max - min > 1) {
		const auto index = (max + min) >> 1;
		const auto diff = (value - m_stack[index]->getZ()).sign();
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

static inline std::size_t AdaptIndex(const cgss::GraphicsContainer& stack, std::size_t index, const cgss::ZIndex& value) {
	if (stack.empty()) {
		return 0;
	}

	if (index == 1) {
		const auto diff = (value - stack[index]->getZ()).sign();
		LOG_DEBUG << "\tAdapting index " << index << " with diff = " << diff << " for value z = " << value.z << ", " << value.index;
		if (diff == -1) {
			return (value - stack[0]->getZ()).sign() <= 0 ? 0 : 1;
		}
		return diff + index;
	}

	const auto diff = (value - stack[index]->getZ()).sign();
	const auto finalIndex = index + (diff <= 0 ? 0 : 1);
	LOG_DEBUG << "\tAdapting index " << index << " to index = " << finalIndex << " for value z = " << value.z << ", " << value.index;
	return finalIndex;
}

std::size_t cgss::ZAlgorithm::findFirstGreater(const ZIndex& index) const {
	return AdaptIndex(m_stack, dichotomicFindFirstGreater(index), index);
}

bool cgss::ZAlgorithm::remove(const GraphicsStackItem* item) {
	const auto index = findFirstGreater(item->getZ());

	const auto foundExactValue = item == m_stack[index].get();
	if (!foundExactValue) {
		LOG_ERROR << static_cast<void*>(this) << " Trying to remove an element from stack but unable to find it for z-index = " << item->getZ().z << ", " << item->getZ().index
			<< " only found position = " << index << " but no item equality";
		LOG_ERROR << *this;
		return false;
	}

	LOG_INFO << "Element removed from stack at index " << index << " for z-index = " << item->getZ().z << ", " << item->getZ().index;

	m_stack.erase(m_stack.begin() + index);
	return true;
}
