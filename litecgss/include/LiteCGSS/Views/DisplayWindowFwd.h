#ifndef CGSS_DISPLAY_WINDOW_FWD_H
#define CGSS_DISPLAY_WINDOW_FWD_H

// Forward-declares `cgss::DisplayWindow` (= `basic_DisplayWindow<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_DisplayWindow;
	using DisplayWindow = basic_DisplayWindow<cgss::backend::ActiveBackend>;
}

#endif
