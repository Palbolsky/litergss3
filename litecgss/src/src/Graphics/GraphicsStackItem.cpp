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

void cgss::GraphicsStackItem::bindRectangle(Rectangle* rect) {
	if (rect != m_rect) {
		auto lastLinkedRect = m_rect;
		m_rect = nullptr;
		if (lastLinkedRect != nullptr) {
			lastLinkedRect->bindElement(nullptr);
		}
		m_rect = rect;
		if (m_rect != nullptr) {
			m_rect->bindElement(this);
		}
	}
}


cgss::GraphicsStackItem::~GraphicsStackItem() {
	bindRectangle(nullptr);
	detach();
}
