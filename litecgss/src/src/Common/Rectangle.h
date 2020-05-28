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

		const sf::IntRect& getRect() const { return m_rect; }

		void bindElement(GraphicsStackItem* target);
		void setRect(sf::IntRect rect);
		void setX(long x);
		void setY(long y);
		void setWidth(long width);
		void setHeight(long height);

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