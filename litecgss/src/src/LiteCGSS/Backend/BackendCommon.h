#ifndef CGSS_BACKEND_COMMON_H
#define CGSS_BACKEND_COMMON_H

// Backend-agnostic POD types used by every backend's Fwd / Ops surface.
// Pulled into both `SFMLBackendFwd.h` and `RaylibBackendFwd.h` so that ops
// like `get_texture_size(...) -> Size2u` mean the same thing on any backend.

namespace cgss::backend {

	// Plain POD for size queries — avoids leaking native vector types
	// (sf::Vector2u under SFML, raylib has no equivalent struct).
	struct Size2u {
		unsigned int x = 0u;
		unsigned int y = 0u;
	};

}

#endif
