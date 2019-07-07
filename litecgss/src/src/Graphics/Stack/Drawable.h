#ifndef LR_DRAWABLE_H
#define LR_DRAWABLE_H
#include <memory>
#include "GraphicsStackItem.h"

namespace lr {
	class Rectangle;
	class Tone;
	using DrawablePtr = std::unique_ptr<GraphicsStackItem>;

	template <class DrawableChild>
	class Drawable : 
		public GraphicsStackItem {
	protected:
		using GraphicsStackItem::GraphicsStackItem;
	
	public:
		virtual ~Drawable() = default;
		template <class View, class ... Args>
		static DrawableChild& create(View& view, Args&& ... args) {
			return view.add(std::unique_ptr<DrawableChild>{ new DrawableChild(std::forward<Args>(args)...) });
		}
	};
	
}
#endif