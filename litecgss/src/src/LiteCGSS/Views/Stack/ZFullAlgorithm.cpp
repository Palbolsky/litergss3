#include <algorithm>
#include "ZFullAlgorithm.h"
#include "LiteCGSS/Graphics/GraphicsStackItem.h"
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::ZFullAlgorithm);

#define LOG_INFO SKA_LOGC_STATIC(cgss::GetLoggerStack(), ska::LogLevel::Info, cgss::ZFullAlgorithm)

void cgss::ZFullAlgorithm::sortZ() {
	if (m_zDirty) {
		std::sort(m_stack.begin(), m_stack.end(), GraphicsStackItem::ZIndexSortPtr);
		m_zDirty = false;
	}
}

void cgss::ZFullAlgorithm::add(GraphicsStackItemPtr item) {
	LOG_INFO << "Adding element to stack " << static_cast<void*>(this) << " with z-index = " <<
		item->getZ().z << ", " << item->getZ().index;
	m_stack.push_back(std::move(item));
	setZDirty();
}

void cgss::ZFullAlgorithm::setZDirty() {
	m_zDirty = true;
}

void cgss::ZFullAlgorithm::set(ZIndex& item, long z) {
	setZDirty();
	item.z = z;
}

bool cgss::ZFullAlgorithm::remove(const GraphicsStackItem* item) {
	sortZ();
	return cgss::ZAlgorithm::remove(item);
}

void cgss::ZFullAlgorithm::refresh() {
	sortZ();
}
