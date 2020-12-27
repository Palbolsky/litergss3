#include "ZInsertionAlgorithm.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::ZInsertionAlgorithm);

#define LOG_DEBUG SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Debug, cgss::ZInsertionAlgorithm)
#define LOG_INFO SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Info, cgss::ZInsertionAlgorithm)

// What this function does :
// RightCircleShift(1, 3, [A, B, C, D, E]) => [A, D, B, C, E]
// RightCircleShift(0, 5, [A, B, C, D, E]) => [E, A, B, C, D]
// Obviously we suppose there is not any out of bounds array access when calling this
void cgss::ZInsertionAlgorithm::rightCircleShift(std::size_t begin, std::size_t end) {
	auto last = std::move(m_stack[end]);
	for(auto index = end; index > begin; index--) {
		m_stack[index] = std::move(m_stack[index - 1]);
	}
	m_stack[begin] = std::move(last);
}

// The opposite of RightCircleShift
void cgss::ZInsertionAlgorithm::leftCircleShift(std::size_t begin, std::size_t end) {
	auto first = std::move(m_stack[begin]);
	for(auto index = begin; index < end; index++) {
		m_stack[index] = std::move(m_stack[index + 1]);
	}
	m_stack[end] = std::move(first);
}

void cgss::ZInsertionAlgorithm::zSwap(const ZIndex& previous, const ZIndex& next) {
	std::size_t range[2] = { findFirstGreater(previous), findFirstGreater(next) };

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

void cgss::ZInsertionAlgorithm::add(GraphicsStackItemPtr item) {
	const auto position = findFirstGreater(item->getZ());
	
	LOG_INFO << "Adding element to stack " << static_cast<void*>(this) << " with z-index = " <<
		item->getZ().z << ", " << item->getZ().index << " at position = " << position;
	if (position == m_stack.size()) {
		m_stack.push_back(std::move(item));
	} else {
		m_stack.insert(m_stack.begin() + position, std::move(item));
	}
}

void cgss::ZInsertionAlgorithm::set(ZIndex& item, long z) {
	auto next = ZIndex{z, item.index};
	zSwap(item, next);
	LOG_DEBUG << "Zswapping to z = " << z;
	item.z = z;
}

bool cgss::ZInsertionAlgorithm::remove(const GraphicsStackItem* item) {
	return cgss::ZAlgorithm::remove(item);
}

void cgss::ZInsertionAlgorithm::refresh() { }
