#ifndef CGSS_COMMON_PROFILER_H
#define CGSS_COMMON_PROFILER_H

// The Tracy profiler of an instrumented build (LITECGSS_USE_TRACY), for a
// host that has no profiler of its own to start (tracy_ruby): Start() before
// any other LiteCGSS call, Stop() at exit. Without instrumentation, Available()
// is false and the rest does nothing. The profiler listens on loopback only.
namespace cgss::profiler {

	bool Available();
	void Start();
	void Stop();

}

#endif
