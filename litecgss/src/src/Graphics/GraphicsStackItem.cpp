#include <cstdio>
#include "GraphicsStackItem.h"
#include "Views/Stack/GraphicsStack.h"
#include "Common/Rectangle.h"

void cgss::GraphicsStackItem::detach() {
	if(m_originStack != nullptr) {
		if (!m_originStack->transferToDetached(this)) {
			//printf and not std::cout because we can be in a noexcept context (when called from destructor)
			printf("WARNING : unable to detach a graphics stack item from his origin stack (element not found)\n");
		}
		m_originStack = nullptr;
	}
}

void cgss::GraphicsStackItem::setZ(ZIndex z) {
	m_originStack->zSwap(m_priority, z);
	m_priority = std::move(z);
}

void cgss::GraphicsStackItem::setZ(long z) {
	m_originStack->zSwap(m_priority, {z, m_priority.index});
	m_priority.z = z;
}

cgss::GraphicsStackItem::~GraphicsStackItem() {
	detach();
}
