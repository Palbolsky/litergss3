#ifndef CGSS_INTERNAL_SF_RECT_CONV_H
#define CGSS_INTERNAL_SF_RECT_CONV_H

// Internal header: included only by implementation .cpp files that need to
// bridge cgss::IntRect with sf::IntRect at the backend boundary. Not part of
// the public API — consumers of LiteCGSS must never include this.

#include <SFML/Graphics/Rect.hpp>
#include "LiteCGSS/Common/IntRect.h"

namespace cgss::internal {

	inline sf::IntRect toSf(const cgss::IntRect& r) noexcept {
		return sf::IntRect(r.left, r.top, r.width, r.height);
	}

	inline cgss::IntRect fromSf(const sf::IntRect& r) noexcept {
		return cgss::IntRect{ r.left, r.top, r.width, r.height };
	}

}

#endif
