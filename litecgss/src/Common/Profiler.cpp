#include "LiteCGSS/Common/Profiler.h"

#ifdef LITECGSS_USE_TRACY
#include <client/TracyProfiler.hpp>

namespace {
	bool g_started = false;
}

bool cgss::profiler::Available() { return true; }

void cgss::profiler::Start() {
	if (g_started) return;
	tracy::StartupProfiler();
	g_started = true;
}

void cgss::profiler::Stop() {
	if (!g_started) return;
	tracy::ShutdownProfiler();
	g_started = false;
}

#else

bool cgss::profiler::Available() { return false; }
void cgss::profiler::Start() {}
void cgss::profiler::Stop() {}

#endif
