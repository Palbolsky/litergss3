#include <SFML/Window/VideoMode.hpp>
#include "LiteCGSS/Common/NormalizeNumbers.h"
#include "DisplayWindowSettings.h"

static cgss::detail::DisplayWindowVideoSettingsData BuildData(
	long bitsPerPixel,
	long width,
	long height,
	double scale) {

	/* Default video mode */
	sf::VideoMode vmode(640, 480, 32);
	
	int bitsPerPixelDefault = 32;
	long maxWidth = 0xFFFFFF;
	long maxHeight = 0xFFFFFF;
	std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
	
	/* If there's a fullscreen mode */
	if (modes.size() > 0) {
		maxWidth = modes[0].width;
		maxHeight = modes[0].height;
		bitsPerPixelDefault = modes[0].bitsPerPixel;
		vmode.bitsPerPixel = modes[0].bitsPerPixel;
	}
	
	/* Adjust Width */
	if (width != -1) {
		vmode.width = cgss::normalize_long(width, 0, maxWidth);
	}

	/* Adjust Height */
	if (height != -1) {
		vmode.height = cgss::normalize_long(height, 0, maxHeight);
	}

	/* Adjust Scale */
	const double normalized_scale = scale < 0.0 ? 1.0 : cgss::normalize_double(scale, 0.1, 10);
	
	/* Adjust Bits per pixel */
	vmode.bitsPerPixel = (bitsPerPixel != -1) ? bitsPerPixelDefault : cgss::normalize_long(bitsPerPixel, 16, bitsPerPixelDefault);
	
	return { vmode.bitsPerPixel, vmode.width, vmode.height, normalized_scale };
}


cgss::DisplayWindowVideoSettings::DisplayWindowVideoSettings(long bitsPerPixel, long width, long height, double scale) :
	DisplayWindowVideoSettings(BuildData(bitsPerPixel, width, height, scale)) {
}

cgss::DisplayWindowVideoSettings::DisplayWindowVideoSettings(detail::DisplayWindowVideoSettingsData source) :
	bitsPerPixel(source.bitsPerPixel),
	width(source.width),
	height(source.height),
	scale(source.scale) {
}

cgss::DisplayWindowVideoSettings::DisplayWindowVideoSettings() :
	DisplayWindowVideoSettings(detail::DisplayWindowVideoSettingsData{}) {
}