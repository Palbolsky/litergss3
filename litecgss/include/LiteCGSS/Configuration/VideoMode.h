#ifndef CGSS_VIDEO_MODE_H
#define CGSS_VIDEO_MODE_H

namespace cgss {

	// A display mode the window can switch to in fullscreen.
	struct VideoMode {
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int bitsPerPixel = 0;
	};

}

#endif
