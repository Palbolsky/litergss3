#include <cstdio>
#include "GraphicsStackItem.h"
#include "LiteCGSS/Views/Stack/GraphicsStack.h"

void cgss::GraphicsStackItem::detach() {
	if(m_originStack != nullptr) {
		if (!m_originStack->remove(this)) {
			//printf and not std::cout because we can be in a noexcept context (when called from destructor)
			printf("WARNING : unable to detach a graphics stack item from his origin stack (element not found)\n");
		}
		m_originStack = nullptr;
	}
}

void cgss::GraphicsStackItem::setZ(long z) {
	if (m_originStack != nullptr) {
		m_originStack->set(m_priority, z);
	} else {
		m_priority.z = z;
	}
}

cgss::GraphicsStackItem::~GraphicsStackItem() {
	detach();
}

void cgss::GraphicsStackItem::tryDraw(sf::RenderTarget& target) const {
	if (m_visible) {
		draw(target);
	}
}

void cgss::GraphicsStackItem::tryDrawFast(sf::RenderTarget& target) const {
	if (m_visible) {
		drawFast(target);
	}
}
