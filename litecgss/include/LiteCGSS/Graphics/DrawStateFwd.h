#ifndef CGSS_DRAW_STATE_FWD_H
#define CGSS_DRAW_STATE_FWD_H

// Forward-declares `cgss::DrawState` (= `basic_DrawState<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_DrawState;
	using DrawState = basic_DrawState<cgss::backend::ActiveBackend>;
}

#endif
