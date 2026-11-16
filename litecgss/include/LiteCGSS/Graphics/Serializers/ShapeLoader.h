#ifndef CGSS_SHAPE_LOADER_H
#define CGSS_SHAPE_LOADER_H

#include <cstddef>

#include "LiteCGSS/Graphics/ShapeGeometry.h"

namespace cgss {

	// Factories for ShapeGeometry. Shape geometry is held as a
	// backend-agnostic POD; callers construct one via these helpers (or
	// directly) instead of building a native shape themselves.
	class ShapeLoader {
	public:
		static ShapeGeometry buildRectangle(float width, float height) {
			ShapeGeometry g;
			g.type = ShapeType::Rectangle;
			g.width = width;
			g.height = height;
			return g;
		}

		static ShapeGeometry buildCircle(float radius, std::size_t numPoints) {
			ShapeGeometry g;
			g.type = ShapeType::Circle;
			g.radius = radius;
			g.pointCount = numPoints;
			return g;
		}

		static ShapeGeometry buildConvex(std::size_t numPoints) {
			ShapeGeometry g;
			g.type = ShapeType::Convex;
			g.pointCount = numPoints;
			g.points.resize(numPoints);
			return g;
		}
	};

}
#endif
