#include "Graphics/LoggerDrawables.h"

#ifdef NDEBUG
#define CGSS_LOG_DRAWABLES nullptr
#else
#define CGSS_LOG_DRAWABLES "Drawables.log"
#endif

cgss::LoggerDrawables& cgss::GetLoggerDrawables() {
	static LoggerDrawables logger{ CGSS_LOG_DRAWABLES };
	return logger;
}
