#ifndef CGSS_VIEWS_FRAME_DUMP_H
#define CGSS_VIEWS_FRAME_DUMP_H

// Out-of-band frame capture for development tools (litecgss-frame-dump):
// the frame as presented — after scale and post-processing, before the
// buffers swap — written as PNG, without the game's scripting layer.
//
//   LITECGSS_DUMP_FRAMES=<dir>[:every=N]
//
// every=N: one frame in N (frames counted per window). Without it, only on
// request: create <dir>/request (any content) and the next frame drawn is
// written, then the request file removed. Files: <dir>/f<frame>-<w>x<h>-s<scale>.png.
// Compiled only with LITECGSS_AGENT (never in a release build); off unless
// the variable is set.

#include <functional>
#include <memory>

#include "LiteCGSS/Graphics/Texture.h"

namespace cgss::detail {

	// Called by DisplayWindow::draw() once per frame, before present.
	// `capture` takes the frame (DisplayWindow::takeSnapshot).
	void frameDumpAfterDraw(const void* window, double scale,
	                        const std::function<std::unique_ptr<cgss::Texture>()>& capture);

}

#endif
