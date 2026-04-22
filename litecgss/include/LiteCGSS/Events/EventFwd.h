#ifndef CGSS_EVENT_FWD_H
#define CGSS_EVENT_FWD_H

// Forward-declares `cgss::Event` (= `basic_Event<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Event;
	using Event = basic_Event<cgss::backend::ActiveBackend>;
}

#endif
