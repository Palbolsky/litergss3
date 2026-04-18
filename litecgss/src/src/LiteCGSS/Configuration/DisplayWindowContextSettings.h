#ifndef CGSS_DISPLAY_WINDOW_CONTEXT_SETTINGS_H
#define CGSS_DISPLAY_WINDOW_CONTEXT_SETTINGS_H

#include <cstdint>

namespace cgss {

	// Backend-agnostic GL context settings. Structurally parallel to sf::ContextSettings
	// (same field names and types) so that conversion at the backend boundary is trivial —
	// see Configuration/Internal/SfContextSettingsConv.h.
	struct DisplayWindowContextSettings {
		enum : std::uint32_t {
			Default = 0,
			Core    = 1 << 0,
			Debug   = 1 << 2
		};

		unsigned int   depthBits          = 0;
		unsigned int   stencilBits        = 0;
		unsigned int   antialiasingLevel  = 0;
		unsigned int   majorVersion       = 1;
		unsigned int   minorVersion       = 1;
		std::uint32_t  attributeFlags     = Default;
		bool           sRgbCapable        = false;
	};

}

#endif
