#ifndef LR_VIEWPORT_H
#define LR_VIEWPORT_H

#include "View.h"
#include "ViewAuthorizations.h"

namespace lr {
	class Viewport : 
		public View,
		public GraphicsStackItem {
	public:
		void draw(sf::RenderTarget& target) const override {}
		void drawFast(sf::RenderTarget& target) const override {}
	};

	class Sprite;
	class Text;
	class Shape;
	class FramedView;

	template <>
	struct ViewAuthorizations<Viewport> : public AuthorizedDrawableSet<Sprite, Text, Shape, FramedView> {};
}

#endif