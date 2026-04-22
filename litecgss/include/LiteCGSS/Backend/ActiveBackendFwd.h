#ifndef CGSS_BACKEND_ACTIVE_BACKEND_FWD_H
#define CGSS_BACKEND_ACTIVE_BACKEND_FWD_H

// Slim backend selector — pulls only the chosen backend's Fwd header
// (typedefs + tags + sf::/raylib::/... forward declarations). Function
// bodies live in the matching full header (e.g. SFMLBackend.h), included
// only by implementation .cpps and via ActiveBackend.h.
//
// Public LiteCGSS templates depend on this header so consumers can name
// `cgss::Texture` (= `basic_Texture<ActiveBackend>`) without their TU
// pulling in <SFML/...>.

#if defined(CGSS_BACKEND_SFML)
#  include "LiteCGSS/Backend/SFMLBackendFwd.h"
namespace cgss::backend { using ActiveBackend = SFMLBackend; }
#elif defined(CGSS_BACKEND_RAYLIB)
#  include "LiteCGSS/Backend/RaylibBackendFwd.h"
namespace cgss::backend { using ActiveBackend = RaylibBackend; }
#elif defined(CGSS_BACKEND_LIBRETRO)
#  include "LiteCGSS/Backend/LibretroBackendFwd.h"
namespace cgss::backend { using ActiveBackend = LibretroBackend; }
#else
#  error "No CGSS_BACKEND_* selected. Configure with -DCGSS_BACKEND=sfml (or raylib, libretro)."
#endif

#endif
