#include <algorithm>
#include "GraphicsStack.h"
#include "Graphics/GraphicsStackItem.h"
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::GraphicsStack);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Debug, cgss::GraphicsStack)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Info, cgss::GraphicsStack)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Warn, cgss::GraphicsStack)

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
    return std::numeric_limits<std::size_t>::max();
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

  const auto finalIndex = (max + min) >> 1;
  const auto diff = (value - m_stack[finalIndex]->getZ()).sign();
  switch (diff) {
  case 1:
      return max;
  default:
  case 0:
  case -1:
      return finalIndex;
  }
}

void cgss::GraphicsStack::zSwap(const ZIndex& previous, const ZIndex& next) {
    LOG_DEBUG << "Stack item zswaping, previous = " << previous.z << ", " << previous.index << " next = " << next.z << ", " << next.index;
    
    if (previous == next) {
      return;
    }
    
    std::size_t range[2];
    range[0] = dichotomicFindFirstGreater(previous);
    range[1] = dichotomicFindFirstGreater(next);
    
    const auto diff = static_cast<long>(range[1] - range[0]);
    if (diff == 0) {
      return;
    }

    if (diff > 0) {
        if (range[1] != m_stack.size() - 1) {
            range[1]--;
        }
        leftCircleShift(range[0], range[1]);
    } else {
        rightCircleShift(range[1], range[0]);
    }
}

void cgss::GraphicsStack::syncFromRawData(GraphicsContainer data, bool forceDetachAll) {
	if(forceDetachAll) {
		for(auto& it : m_stack) {
			it->forceDetach();
		}
	}
	m_stack = std::move(data);
}

bool cgss::GraphicsStack::remove(GraphicsStackItem* item) {
    const auto index = dichotomicFindFirstGreater(item->getZ());
    const auto foundExactValue = item == m_stack[index].get();
    if (!foundExactValue) {
        LOG_WARN << "Trying to remove an element from stack but unable to find it";
        return false;
    }

    LOG_INFO << "Element removed from stack at index " << index << " for z-index = " << item->getZ().z << ", " << item->getZ().index;

    auto it = m_stack.begin();
    std::advance(it, index);

    m_stack.erase(it);
	return true;
}

void cgss::GraphicsStack::add(GraphicsStackItemPtr item) {
    item->forceAttach(*this);
    m_zIndexCounter++;

    LOG_INFO << "Adding element to stack with z-index = " << item->getZ().z << ", " << item->getZ().index;
	m_stack.push_back(std::move(item));
}
