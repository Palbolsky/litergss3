#include "LoggerViews.h"

cgss::LoggerViews& cgss::GetLoggerViews() {
    static LoggerViews viewsLogger{ "Views.log" };
    return viewsLogger;
}
