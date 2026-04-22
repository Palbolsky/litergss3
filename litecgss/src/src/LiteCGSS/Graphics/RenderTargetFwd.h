#ifndef CGSS_RENDER_TARGET_FWD_H
#define CGSS_RENDER_TARGET_FWD_H

// Forward-declares `cgss::RenderTarget` (= `basic_RenderTarget<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_RenderTarget;
	using RenderTarget = basic_RenderTarget<cgss::backend::ActiveBackend>;
}

#endif
