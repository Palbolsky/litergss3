#ifndef CGSS_CAMERA_FWD_H
#define CGSS_CAMERA_FWD_H

// Forward-declares `cgss::Camera` (= `basic_Camera<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Camera;
	using Camera = basic_Camera<cgss::backend::ActiveBackend>;
}

#endif
