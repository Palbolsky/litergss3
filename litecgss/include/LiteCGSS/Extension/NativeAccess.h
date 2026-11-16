#ifndef CGSS_EXTENSION_NATIVE_ACCESS_H
#define CGSS_EXTENSION_NATIVE_ACCESS_H

// Extension SPI — opt-in escape hatch for modules that legitimately need
// direct access to the native backend handle inside a cgss wrapper. Examples:
// video decoders writing decoded frames into a cgss::Texture, platform-
// specific capture or effect plugins, native-interop tooling.
//
// Opt-in by linking the `LiteCGSS::extension_spi` CMake target (instead of
// `LiteCGSS::engine`): it defines CGSS_ENABLE_EXTENSION_SPI and exposes the
// private backend headers this file needs. Without it, this file is an empty
// include — keeps the extension surface from silently bleeding into portable
// code. This is the only public header allowed to include a private one.
//
// Usage:
//     target_link_libraries(my_plugin PRIVATE LiteCGSS::extension_spi)
//     #include "LiteCGSS/Extension/NativeAccess.h"
//
//     cgss::Texture tex = ...;
//     auto& raw = cgss::extension::native(tex);  // the active backend's
//                                                // native_texture
//
// WARNINGS:
// - Any TU that calls cgss::extension::native(...) is STRUCTURALLY coupled
//   to a specific backend's native types. It will not build against other
//   backends. Module authors should gate their CMake targets to the backend
//   they support, or use `if constexpr` with the backend capability tags.
// - No guarantee is made about the lifetime semantics of the returned
//   reference — it is valid only as long as the passed wrapper outlives it.
// - For pixel-update use cases (the canonical SFMovie pattern), prefer
//   `cgss::Texture::update(image)` + the existing `Ops::texture_update_pixels`
//   path. That already works without an escape hatch.

#ifdef CGSS_ENABLE_EXTENSION_SPI

#include "Backend/ActiveBackend.h"

namespace cgss::extension {

	// Return a reference to the native handle inside a cgss wrapper.
	// Routes through the backend's Ops::native_of which is friend of every
	// cgss wrapper — so this template works uniformly for Texture, Image,
	// Event, Font, Camera, etc. (any wrapper with the standard friend).
	template <class W>
	inline auto& native(W& wrapper) noexcept {
		return cgss::backend::ActiveBackend::Ops::native_of(wrapper);
	}

	template <class W>
	inline const auto& native(const W& wrapper) noexcept {
		return cgss::backend::ActiveBackend::Ops::native_of(wrapper);
	}

}

#endif // CGSS_ENABLE_EXTENSION_SPI

#endif
