#ifndef CGSS_PROFILING_H
#define CGSS_PROFILING_H

// Profiler instrumentation (Tracy), compiled in with LITECGSS_USE_TRACY and
// to nothing otherwise. With it on, the embedder must start Tracy's client
// before LiteCGSS runs (TRACY_MANUAL_LIFETIME: tracy::StartupProfiler()).
//
//   CGSS_PROFILE_ZONE("cgss::Class::method");   // scoped to the enclosing block
//   CGSS_PROFILE_FRAME();                        // a frame was presented

#ifdef LITECGSS_USE_TRACY
#include <tracy/Tracy.hpp>
#define CGSS_PROFILE_ZONE(name) ZoneScopedN(name)
#define CGSS_PROFILE_FRAME() FrameMark
#else
#define CGSS_PROFILE_ZONE(name)
#define CGSS_PROFILE_FRAME()
#endif

#endif
