#ifndef LR_GRAPHICS_STACK_ITEM_PTR_H
#define LR_GRAPHICS_STACK_ITEM_PTR_H
#include <memory>

namespace lr {
	class GraphicsStackItem;
	using GraphicsStackItemPtr = std::unique_ptr<GraphicsStackItem>;
}
#endif
