#ifndef CGSS_TEXTURE_FWD_H
#define CGSS_TEXTURE_FWD_H

// Forward-declares `cgss::Texture` (= `basic_Texture<ActiveBackend>`) for
// other public headers that previously did `class Texture;`. After PR C
// `cgss::Texture` is a template alias rather than a class, so a bare
// forward declaration would conflict — include this instead.

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Texture;
	using Texture = basic_Texture<cgss::backend::ActiveBackend>;
}

#endif
