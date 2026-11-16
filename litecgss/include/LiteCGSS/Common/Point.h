#ifndef CGSS_POINT_H
#define CGSS_POINT_H

namespace cgss {

	// 2D float point — backend-agnostic POD, the public return type of
	// Shape::getPoint() and similar.
	struct Point {
		float x = 0.f;
		float y = 0.f;
	};

	// 2D integer point, in pixels.
	struct IntPoint {
		int x = 0;
		int y = 0;
	};

}

#endif
