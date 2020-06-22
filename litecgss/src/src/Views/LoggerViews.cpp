#include "LoggerViews.h"

#ifdef NDEBUG
#define CGSS_LOG_VIEWS nullptr
#else
#define CGSS_LOG_VIEWS "Views.log"
#endif

cgss::LoggerViews& cgss::GetLoggerViews() {
    static LoggerViews viewsLogger{ CGSS_LOG_VIEWS };
    return viewsLogger;
}
