#ifndef CGSS_RECTANGLE_H
#define CGSS_RECTANGLE_H
#include <SFML/Graphics.hpp>
#include "Meta/metadata.h"

namespace cgss {
	class GraphicsStackItem;
	class Rectangle {
	private:
		sf::IntRect m_rect;
		GraphicsStackItem* m_target = nullptr;
		void updateLinkedElement();
	public:
		Rectangle() = default;
		virtual ~Rectangle();

		sf::IntRect& getRect() { return m_rect; };
		const sf::IntRect& getRect() const { return m_rect; }

		void bindElement(GraphicsStackItem* target);
		void setRect(sf::IntRect rect);

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