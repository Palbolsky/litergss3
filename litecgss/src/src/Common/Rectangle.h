#ifndef LR_RECTANGLE_H
#define LR_RECTANGLE_H
#include <SFML/Graphics.hpp>
#include "Graphics/Stack/GraphicsStackItem.h"
#include "Meta/metadata.h"

namespace lr {
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

		sf::IntRect& getRect() { return m_rect; }
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