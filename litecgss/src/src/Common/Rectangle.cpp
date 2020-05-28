#include "Rectangle.h"
#include "Graphics/GraphicsStackItem.h"

void cgss::Rectangle::updateLinkedElement() {
	if (m_target == nullptr) {
		return;
  }

	m_target->updateFromRectangle(m_rect);
}

void cgss::Rectangle::setRect(sf::IntRect rect) {
  m_rect = std::move(rect);
  updateLinkedElement();
}

cgss::Rectangle::~Rectangle() {
  if (m_target != nullptr) {
    m_target->bindRectangle(nullptr);
  }
}

void cgss::Rectangle::bindElement(GraphicsStackItem* target) {
  if (target != m_target) {
    auto lastLinkedElement = m_target;
    m_target = nullptr;
    if (lastLinkedElement != nullptr) {
      lastLinkedElement->bindRectangle(nullptr);
    }
    m_target = target;
    if (m_target != nullptr) {
      m_target->bindRectangle(this);
    }
  }
}
