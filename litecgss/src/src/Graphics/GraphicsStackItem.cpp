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
	m_originStack->zSwap(m_priority, {z, m_priority.index});
	LOG_DEBUG << "Zswapping to z = " << z;
	m_priority.z = z;
}

cgss::GraphicsStackItem::~GraphicsStackItem() {
	detach();
	bindRectangle(nullptr);
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

void cgss::GraphicsStackItem::bindRectangle(Rectangle* rectangle) {
	if (rectangle != m_boundRectangle) {
		auto lastLinkedRect = m_boundRectangle;
		m_boundRectangle = nullptr;
		if (lastLinkedRect != nullptr) {
			lastLinkedRect->bindElement(nullptr);
		}
		m_boundRectangle = rectangle;
		if (m_boundRectangle != nullptr) {
			m_boundRectangle->bindElement(this);
		}
	}
}
