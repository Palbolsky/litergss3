#ifndef CGSS_INTERNAL_SF_COLOR_CONV_H
#define CGSS_INTERNAL_SF_COLOR_CONV_H

// Internal header: included only by implementation .cpp files that need to
// bridge cgss::Color with sf::Color at the backend boundary. Not part of
// the public API — consumers of LiteCGSS must never include this.

#include <SFML/Graphics/Color.hpp>
#include "LiteCGSS/Common/Color.h"

namespace cgss::internal {

	inline sf::Color toSf(const cgss::Color& c) noexcept {
		return sf::Color(c.r, c.g, c.b, c.a);
	}

	inline cgss::Color fromSf(const sf::Color& c) noexcept {
		return cgss::Color{ c.r, c.g, c.b, c.a };
	}

}

#endif
