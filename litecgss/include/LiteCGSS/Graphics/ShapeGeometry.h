#ifndef CGSS_SHAPE_GEOMETRY_H
#define CGSS_SHAPE_GEOMETRY_H

#include <vector>
#include <cstddef>

#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Point.h"

namespace cgss {

	enum class ShapeType {
		Rectangle,
		Circle,
		Convex
	};

	// Backend-agnostic shape geometry + material state. ShapeItem owns one of
	// these; the active backend constructs the corresponding native primitive
	// at draw time. No native graphics state is
	// retained between draws, so the same geometry renders identically under
	// any backend that implements `Backend::Ops::draw_shape`.
	//
	// Field-by-shape-type usage:
	//   Rectangle: `width`, `height`
	//   Circle:    `radius`, `pointCount`
	//   Convex:    `points` (and pointCount derived from points.size())
	// Material (fill / outline / texture rect) applies to all types.
	struct ShapeGeometry {
		ShapeType type = ShapeType::Rectangle;

		// Material
		cgss::Color  fillColor        = cgss::Colors::White;
		cgss::Color  outlineColor     = cgss::Colors::Black;
		float        outlineThickness = 0.f;
		cgss::IntRect textureRect     = { 0, 0, 0, 0 };

		// Geometry
		float width  = 0.f;
		float height = 0.f;
		float radius = 0.f;
		std::size_t pointCount = 30;            // default for Circle
		std::vector<cgss::Point> points;        // for Convex
	};

}

#endif
