#ifndef CGSS_IMAGE_FWD_H
#define CGSS_IMAGE_FWD_H

// Forward-declares `cgss::Image` (= `basic_Image<ActiveBackend>`).

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Image;
	using Image = basic_Image<cgss::backend::ActiveBackend>;
}

#endif
