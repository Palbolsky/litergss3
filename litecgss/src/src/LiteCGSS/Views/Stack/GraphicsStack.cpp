#include <algorithm>
#include "GraphicsStack.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"
#include "LoggerStack.h"
#include "LiteCGSS/Graphics/ZIndex.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::GraphicsStack);

#define LOG_DEBUG SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Debug, cgss::GraphicsStack)
#define LOG_INFO SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Info, cgss::GraphicsStack)
#define LOG_WARN SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Warn, cgss::GraphicsStack)
#define LOG_ERROR SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Error, cgss::GraphicsStack)

cgss::GraphicsStack::~GraphicsStack() {
	clear();
	LOG_INFO << "Stack destructed";
}

void cgss::GraphicsStack::clear() {
	syncFromRawData({});
	LOG_INFO << "Stack cleared";
}

// What this function does :
// RightCircleShift(1, 3, [A, B, C, D, E]) => [A, D, B, C, E]
// RightCircleShift(0, 5, [A, B, C, D, E]) => [E, A, B, C, D]
// Obviously we suppose there is not any out of bounds array access when calling this
void cgss::GraphicsStack::rightCircleShift(std::size_t begin, std::size_t end) {
	auto last = std::move(m_stack[end]);
	for(auto index = end; index > begin; index--) {
		m_stack[index] = std::move(m_stack[index - 1]);
	}
	m_stack[begin] = std::move(last);
}

// The opposite of RightCircleShift
void cgss::GraphicsStack::leftCircleShift(std::size_t begin, std::size_t end) {
	auto first = std::move(m_stack[begin]);
	for(auto index = begin; index < end; index++) {
		m_stack[index] = std::move(m_stack[index + 1]);
	}
	m_stack[end] = std::move(first);
}

std::size_t cgss::GraphicsStack::dichotomicFindFirstGreater(const ZIndex& value) const {
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

void cgss::GraphicsStack::zSwap(const ZIndex& previous, const ZIndex& next) {
	std::size_t range[2];
	range[0] = AdaptIndex(m_stack, dichotomicFindFirstGreater(previous), previous);
	range[1] = AdaptIndex(m_stack, dichotomicFindFirstGreater(next), next);

	const auto diff = static_cast<long>(range[1] - range[0]);
	if (diff > 0) {
		range[1]--;
		leftCircleShift(range[0], range[1]);
	} else if (diff < 0) {
		rightCircleShift(range[1], range[0]);
	}

	LOG_DEBUG << "Stack item zswaping, previous = " << previous.z << ", " << previous.index << " found at index = " << range[0]
	<< " next = " << next.z << ", " << next.index << " found at index = " << range[1];
}

static bool ZIndexSort(const cgss::GraphicsStackItemPtr &lhs, const cgss::GraphicsStackItemPtr &rhs) { return lhs->getZ() < rhs->getZ(); }

void cgss::GraphicsStack::sortZ() {
	if (m_zDirty) {
		std::sort(m_stack.begin(), m_stack.end(), ZIndexSort);
		m_zDirty = false;
	}
}

void cgss::GraphicsStack::setZDirty() {
	m_zDirty = true;
}

void cgss::GraphicsStack::syncFromRawData(GraphicsContainer data, bool forceDetachAll) {
	if (forceDetachAll) {
		for(auto& it : m_stack) {
			it->forceDetach();
		}
	}
	m_stack = std::move(data);
}

static std::string PrintStack(const cgss::GraphicsStack& drawables) {
	auto ss = std::stringstream {};
	ss << static_cast<const void*>(&drawables) << "\t";
	for (const auto& element : drawables) {
		ss << "[" << element->getZ().z  << ", " << element->getZ().index << "]";
	}
	return ss.str();
}

bool cgss::GraphicsStack::remove(GraphicsStackItem* item) {
	sortZ();
	const auto index = AdaptIndex(m_stack, dichotomicFindFirstGreater(item->getZ()), item->getZ());

	const auto foundExactValue = item == m_stack[index].get();
	if (!foundExactValue) {
		LOG_ERROR << static_cast<void*>(this) << " Trying to remove an element from stack but unable to find it for z-index = " << item->getZ().z << ", " << item->getZ().index
			<< " only found position = " << index << " but no item equality";
		LOG_ERROR << PrintStack(*this);
		return false;
	}

	LOG_INFO << "Element removed from stack at index " << index << " for z-index = " << item->getZ().z << ", " << item->getZ().index;

	m_stack.erase(m_stack.begin() + index);
	return true;
}

std::int32_t cgss::GraphicsStack::queryNextIndex() {
	return m_zIndexCounter++;
}

void cgss::GraphicsStack::add(GraphicsStackItemPtr item) {
	item->forceAttach(*this);
	if (!m_insertionZSort) {
		LOG_INFO << "Adding element to stack " << static_cast<void*>(this) << " with z-index = " <<
			item->getZ().z << ", " << item->getZ().index;
		m_stack.push_back(std::move(item));
		m_zDirty = true;
		return;
	}

	const auto position = AdaptIndex(m_stack, dichotomicFindFirstGreater(item->getZ()), item->getZ());
	
	LOG_INFO << "Adding element to stack " << static_cast<void*>(this) << " with z-index = " <<
		item->getZ().z << ", " << item->getZ().index << " at position = " << position;
	if (position == m_stack.size()) {
		m_stack.push_back(std::move(item));
	} else {
		m_stack.insert(m_stack.begin() + position, std::move(item));
	}
}

bool cgss::GraphicsStack::isInsertionZSort() const {
	return m_insertionZSort;
}

void cgss::GraphicsStack::setInsertionZSort(bool zsort) {
	m_insertionZSort = zsort;
}
