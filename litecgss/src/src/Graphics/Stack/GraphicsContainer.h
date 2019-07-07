#ifndef LR_GRAPHICS_CONTAINER_H
#define LR_GRAPHICS_CONTAINER_H
#include "Common/vector_tracker.h"
#include "GraphicsStackItemPtr.h"

namespace lr {
	using GraphicsContainer = vector_tracker<GraphicsStackItemPtr>;
}

#endif