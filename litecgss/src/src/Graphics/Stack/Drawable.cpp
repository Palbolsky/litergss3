#include "Drawable.h"
#include "Common/Rectangle.h"

void lr::Drawable::bindRectangle(Rectangle* rect) {
	if(rect != m_rect) {
		auto lastLinkedRect = m_rect;
		m_rect = nullptr;
		if(lastLinkedRect != nullptr) {
			lastLinkedRect->bindElement(nullptr);
		}
		m_rect = rect;
		if(m_rect != nullptr) {
			m_rect->bindElement(this);
		}
	}
}


lr::Drawable::~Drawable() {
	bindRectangle(nullptr);
}
