#ifndef CGSS_DRAWABLE_STACK_H
#define CGSS_DRAWABLE_STACK_H

#include <cstdint>
#include <SFML/Graphics.hpp>
#include "GraphicsStack.h"

namespace cgss {
	class DrawableStack :
		public GraphicsStack {
	public:
		using GraphicsStack::GraphicsStack;
		void draw(sf::View& defview, sf::RenderTarget& target) const;
		void drawFast(sf::RenderTarget& target) const;
		void updateContentsOpacity(std::uint8_t opacity);
	};
}
#endif