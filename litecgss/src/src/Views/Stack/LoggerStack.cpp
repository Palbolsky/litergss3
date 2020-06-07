#include "LoggerStack.h"

cgss::LoggerStack& cgss::GetLoggerStack() {
    static LoggerStack logger{ "Stack.log" };
    return logger;
}
