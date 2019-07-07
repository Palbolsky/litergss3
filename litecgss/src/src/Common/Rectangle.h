#ifndef CGSS_RECTANGLE_H
#define CGSS_RECTANGLE_H
#include <SFML/Graphics.hpp>
#include "Graphics/GraphicsStackItem.h"
#include "Meta/metadata.h"

namespace cgss {
	class Rectangle {
	private:
		sf::IntRect m_rect;
		GraphicsStackItem* m_target = nullptr;
	public:
		Rectangle() = default;
		~Rectangle() {
			if(m_target != nullptr) {
				m_target->bindRectangle(nullptr);
			}
		}

		void setRect(sf::IntRect rect) { m_rect = std::move(rect); }
		const sf::IntRect& getRect() const { return m_rect; }

		void bindElement(GraphicsStackItem* target) {
			if(target != m_target) {
				auto lastLinkedElement = m_target;
				m_target = nullptr;
				if(lastLinkedElement != nullptr) {
					lastLinkedElement->bindRectangle(nullptr);
				}
				m_target = target;
				if(m_target != nullptr) {
					m_target->bindRectangle(this);
				}
			}
		}
		GraphicsStackItem* getElement() { return m_target; }
	};

	namespace meta {
		template<>
		struct Log<Rectangle> {
			static constexpr auto classname = "Rectangle";
		};
	}

}

#endif