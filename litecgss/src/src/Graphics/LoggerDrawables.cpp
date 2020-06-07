#include "LoggerDrawables.h"

cgss::LoggerDrawables& cgss::GetLoggerDrawables() {
    static LoggerDrawables logger{ /*"Drawables.log"*/ nullptr };
    return logger;
}
