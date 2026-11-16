#ifndef CGSS_BACKEND_INTERNAL_SFML_CAPABILITIES_H
#define CGSS_BACKEND_INTERNAL_SFML_CAPABILITIES_H

#include <SFML/Graphics/Texture.hpp>
#include <type_traits>

namespace cgss::backend::sfml_caps {

// Detect at compile time whether the SFML build exposes
// sf::Texture::isNonPowerOfTwoSupported() — a static method added by our
// SFML fork (SFML-opengl-es2). Upstream SFML does not have it, in which
// case we fall back to assuming NPOT is supported (true on every modern
// desktop GL target SFML compiles for; only GL ES 2 without OES_texture_npot
// would actually report false, and that path only matters when the fork
// with the matching shader-side correction is in use anyway).
//
// Implemented via a primary template + SFINAE-gated specialization rather
// than `if constexpr`: the latter only discards branches inside an enclosing
// templated entity, so used directly in a non-template function it would
// still name-lookup the missing member and fail to compile.
template <typename T, typename = void>
struct npot_query {
	static bool run() { return true; }
};

template <typename T>
struct npot_query<T, std::void_t<decltype(T::isNonPowerOfTwoSupported())>> {
	static bool run() { return T::isNonPowerOfTwoSupported(); }
};

inline bool isNonPowerOfTwoSupported() {
	return npot_query<sf::Texture>::run();
}

} // namespace cgss::backend::sfml_caps

#endif
