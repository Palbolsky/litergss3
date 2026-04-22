#ifndef CGSS_BACKEND_ACTIVE_BACKEND_H
#define CGSS_BACKEND_ACTIVE_BACKEND_H

// Full active backend: pulls the alias `cgss::backend::ActiveBackend` and
// the slim Fwd descriptor from ActiveBackendFwd.h, then adds the matching
// full backend header (which defines `Backend::Ops` with all inline thunks).
//
// Include this only from implementation .cpp files. Public templates use
// ActiveBackendFwd.h to keep `<SFML/...>` (or other backend headers) out
// of consumer translation units.

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Backend/BackendConcept.h"

#if defined(CGSS_BACKEND_SFML)
#  include "LiteCGSS/Backend/SFMLBackend.h"
#elif defined(CGSS_BACKEND_RAYLIB)
#  include "LiteCGSS/Backend/RaylibBackend.h"
#elif defined(CGSS_BACKEND_LIBRETRO)
#  include "LiteCGSS/Backend/LibretroBackend.h"
#endif
// No #else — ActiveBackendFwd.h already errors if no CGSS_BACKEND_* is set.

namespace cgss::backend {
	static_assert(is_backend_v<ActiveBackend>,
		"Selected backend does not satisfy cgss::backend::Backend: "
		"check required typedefs and capability tags are present.");
}

#endif
