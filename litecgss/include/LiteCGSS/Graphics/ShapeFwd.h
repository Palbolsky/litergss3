#ifndef CGSS_SHAPE_FWD_H
#define CGSS_SHAPE_FWD_H

// Forward-declares `cgss::Shape` and `cgss::ShapeItem` (= aliases of the
// templated basic_Shape/basic_ShapeItem<ActiveBackend>) for other public
// headers that previously did `class Shape;`.

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Shape;
	template <class B> class basic_ShapeItem;
	using Shape = basic_Shape<cgss::backend::ActiveBackend>;
	using ShapeItem = basic_ShapeItem<cgss::backend::ActiveBackend>;
}

#endif
