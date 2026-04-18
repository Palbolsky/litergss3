#ifndef CGSS_INTERNAL_SF_CONTEXT_SETTINGS_CONV_H
#define CGSS_INTERNAL_SF_CONTEXT_SETTINGS_CONV_H

// Internal header: included only by implementation .cpp files that need to
// bridge cgss::DisplayWindowContextSettings with sf::ContextSettings at the
// backend boundary. Not part of the public API.

#include <SFML/Window/ContextSettings.hpp>
#include "LiteCGSS/Configuration/DisplayWindowContextSettings.h"

namespace cgss::internal {

	inline sf::ContextSettings toSf(const cgss::DisplayWindowContextSettings& c) noexcept {
		return sf::ContextSettings(
			c.depthBits,
			c.stencilBits,
			c.antialiasingLevel,
			c.majorVersion,
			c.minorVersion,
			c.attributeFlags,
			c.sRgbCapable
		);
	}

	inline cgss::DisplayWindowContextSettings fromSf(const sf::ContextSettings& c) noexcept {
		return cgss::DisplayWindowContextSettings{
			c.depthBits,
			c.stencilBits,
			c.antialiasingLevel,
			c.majorVersion,
			c.minorVersion,
			c.attributeFlags,
			c.sRgbCapable
		};
	}

}

#endif
