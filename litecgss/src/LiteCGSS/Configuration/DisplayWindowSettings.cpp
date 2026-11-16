#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/NormalizeNumbers.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"

static cgss::detail::DisplayWindowVideoSettingsData BuildData(
	long bitsPerPixel,
	long width,
	long height,
	double scale) {

	/* Default video mode */
	unsigned int vmWidth = 640;
	unsigned int vmHeight = 480;
	unsigned int vmBitsPerPixel = 32;

	// bitsPerPixel is no longer queryable at the backend layer (raylib has
	// no equivalent of sf::VideoMode::bitsPerPixel); modern displays are
	// effectively always 32-bit, so we keep that as the implicit default.
	const int bitsPerPixelDefault = 32;
	long maxWidth = 0xFFFFFF;
	long maxHeight = 0xFFFFFF;
	const auto desktop = cgss::backend::ActiveBackend::Ops::desktop_size();

	if (desktop.x > 0 && desktop.y > 0) {
		maxWidth = desktop.x;
		maxHeight = desktop.y;
	}

	/* Adjust Width */
	if (width != -1) {
		vmWidth = cgss::normalize_long(width, 0, maxWidth);
	}

	/* Adjust Height */
	if (height != -1) {
		vmHeight = cgss::normalize_long(height, 0, maxHeight);
	}

	/* Adjust Scale */
	const double normalized_scale = scale < 0.0 ? 1.0 : cgss::normalize_double(scale, 0.1, 10);

	/* Adjust Bits per pixel */
	vmBitsPerPixel = (bitsPerPixel != -1) ? bitsPerPixelDefault : cgss::normalize_long(bitsPerPixel, 16, bitsPerPixelDefault);

	return { vmBitsPerPixel, vmWidth, vmHeight, normalized_scale };
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