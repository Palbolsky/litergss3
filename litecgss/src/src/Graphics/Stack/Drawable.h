#ifndef LR_DRAWABLE_H
#define LR_DRAWABLE_H
#include <algorithm>
#include "GraphicsStackItem.h"

namespace lr {
	class Rectangle;
	class Tone;
	class Drawable :
		public GraphicsStackItem {
	private:
		Rectangle* m_rect = nullptr;

	public:
		Drawable() = default;
		virtual ~Drawable();

		void bindRectangle(Rectangle* rect);
		Rectangle* getRectangle() const { return m_rect; };
	};
}
#endif