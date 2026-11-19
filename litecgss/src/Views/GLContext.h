#ifndef CGSS_VIEWS_GL_CONTEXT_H
#define CGSS_VIEWS_GL_CONTEXT_H

namespace cgss::detail {
	// Called by a backend before its GL work. Once the host hands the GL
	// context out on use (DisplayWindow::AcquireContextOnUse), the calling
	// thread takes it here, waiting for a frame in flight on another thread
	// if needed; it keeps it until it lets Ruby's GVL go. Otherwise (and
	// when the thread holds it already) nothing to do: a check of a
	// thread-local flag.
	void ensureGLContext();
}

#endif
