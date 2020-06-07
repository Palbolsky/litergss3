#ifndef CGSS_LOGGER_DRAWABLES_H
#define CGSS_LOGGER_DRAWABLES_H
#include "Common/Logger.h"

namespace cgss {
    using LoggerDrawables = Logger<ska::LogLevel::Debug>;
    LoggerDrawables& GetLoggerDrawables();
}

#endif