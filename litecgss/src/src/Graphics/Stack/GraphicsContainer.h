#ifndef LR_GRAPHICS_CONTAINER_H
#define LR_GRAPHICS_CONTAINER_H

#include "Common/vector_tracker.h"

namespace lr {
	class GraphicsStackItem;
	using GraphicsContainer = vector_tracker<GraphicsStackItem*>;
}

#endif