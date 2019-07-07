#ifndef CGSS_GRAPHICS_CONTAINER_H
#define CGSS_GRAPHICS_CONTAINER_H
#include "Common/vector_tracker.h"
#include "Graphics/GraphicsStackItemPtr.h"

namespace cgss {
	using GraphicsContainer = vector_tracker<GraphicsStackItemPtr>;
}

#endif