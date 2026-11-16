#ifndef CGSS_BACKEND_INTERNAL_RAYLIB_PLATFORM_QUERIES_H
#define CGSS_BACKEND_INTERNAL_RAYLIB_PLATFORM_QUERIES_H

// Queries raylib's public API doesn't offer, answered by the GLFW raylib
// bundles (desktop builds): the fullscreen video modes and the GL maximum
// texture size. GLFW is reached through weak references, so a raylib build
// that doesn't export it still links: the answers then fall back to the
// current monitor's size and the GL 3.3 guaranteed minimum.

#include <vector>

#include "LiteCGSS/Configuration/VideoMode.h"

namespace cgss::backend::raylib_detail {

	std::vector<cgss::VideoMode> fullscreenModes();
	unsigned int maxTextureSize();
	// GL_MAX_TEXTURE_SIZE as the driver reports it, 0 when it can't be
	// queried here (maxTextureSize then answers the GL minimum guarantee).
	unsigned int queriedMaxTextureSize();

}

#endif
