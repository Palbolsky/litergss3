#ifndef CGSS_FONT_FWD_H
#define CGSS_FONT_FWD_H

// Forward-declares `cgss::Font` (= `basic_Font<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Font;
	using Font = basic_Font<cgss::backend::ActiveBackend>;
}

#endif
