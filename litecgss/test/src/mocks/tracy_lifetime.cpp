// Linked into every test executable. A Tracy build (LITECGSS_USE_TRACY) runs
// with TRACY_MANUAL_LIFETIME: the host must start the profiler before any
// instrumented LiteCGSS code runs, and the tests are that host.
#ifdef LITECGSS_USE_TRACY
#include <client/TracyProfiler.hpp>

namespace {
	struct TracyLifetime {
		TracyLifetime() { tracy::StartupProfiler(); }
		~TracyLifetime() { tracy::ShutdownProfiler(); }
	} g_tracyLifetime;
}
#endif
