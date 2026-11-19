#ifndef LITERGSS_PROFILING_H
#define LITERGSS_PROFILING_H

// Tracy zones in the extension, as LiteCGSS's (Common/Profiling.h): compiled
// in with LITECGSS_USE_TRACY (the engine target passes the define and Tracy's
// client on), to nothing otherwise. The steps of a window update the engine
// doesn't see: the agent channel, the bitmap uploads, the wait for the GL
// context, the events handed to Ruby.
//
//   LITERGSS_PROFILE_ZONE("LiteRGSS::DisplayWindow#update");

#ifdef LITECGSS_USE_TRACY
#include <tracy/Tracy.hpp>
#define LITERGSS_PROFILE_ZONE(name) ZoneScopedN(name)
#else
#define LITERGSS_PROFILE_ZONE(name)
#endif

#endif
