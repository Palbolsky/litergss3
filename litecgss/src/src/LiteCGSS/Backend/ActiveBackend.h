#ifndef CGSS_BACKEND_ACTIVE_BACKEND_H
#define CGSS_BACKEND_ACTIVE_BACKEND_H

// Selects the graphics backend for this build. Exactly one of the
// CGSS_BACKEND_* preprocessor macros must be defined — set by the
// CGSS_BACKEND CMake option (default: sfml). See root CMakeLists.txt.
//
// The single-named alias `cgss::backend::ActiveBackend` is the only place
// the backend is referred to by name; wrapper templates take `Backend` as
// a parameter and get specialized to `ActiveBackend` via
// `using X = basic_X<ActiveBackend>` aliases in each public header.

#include "LiteCGSS/Backend/BackendConcept.h"

#if defined(CGSS_BACKEND_SFML)
#  include "LiteCGSS/Backend/SFMLBackend.h"
namespace cgss::backend { using ActiveBackend = SFMLBackend; }
#elif defined(CGSS_BACKEND_RAYLIB)
#  include "LiteCGSS/Backend/RaylibBackend.h"
namespace cgss::backend { using ActiveBackend = RaylibBackend; }
#elif defined(CGSS_BACKEND_LIBRETRO)
#  include "LiteCGSS/Backend/LibretroBackend.h"
namespace cgss::backend { using ActiveBackend = LibretroBackend; }
#else
#  error "No CGSS_BACKEND_* selected. Configure with -DCGSS_BACKEND=sfml (or raylib, libretro)."
#endif

namespace cgss::backend {
	static_assert(is_backend_v<ActiveBackend>,
		"Selected backend does not satisfy cgss::backend::Backend: "
		"check required typedefs and capability tags are present.");
}

#endif
