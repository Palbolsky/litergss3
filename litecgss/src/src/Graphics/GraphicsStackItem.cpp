#include <cstdio>
#include "GraphicsStackItem.h"
#include "Views/Stack/GraphicsStack.h"
#include "Common/Rectangle.h"
#include "LoggerDrawables.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::GraphicsStackItem);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::GraphicsStackItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info, cgss::GraphicsStackItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn, cgss::GraphicsStackItem)

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
		if (m_originStack->isInsertionZSort()) {		
			auto next = ZIndex{z, m_priority.index};
			m_originStack->zSwap(m_priority, next);
			LOG_DEBUG << "Zswapping to z = " << z;
		} else {
			m_originStack->setZDirty();
		}
	}
	m_priority.z = z;
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
