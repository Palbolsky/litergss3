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

void cgss::Rectangle::setX(long x) {
  m_rect.left = x;
  updateLinkedElement();
}

void cgss::Rectangle::setY(long y) {
  m_rect.top = y;
  updateLinkedElement();
}

void cgss::Rectangle::setWidth(long width) {
  m_rect.width = width;
  updateLinkedElement();
}

void cgss::Rectangle::setHeight(long height) {
  m_rect.height = height;
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

bool cgss::Rectangle::operator==(const Rectangle& rect) const {
  return m_rect == rect.m_rect;
}

bool cgss::Rectangle::operator!=(const Rectangle& rect) const {
  return m_rect != rect.m_rect;
}