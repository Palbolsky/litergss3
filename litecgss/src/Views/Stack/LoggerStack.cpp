#include "LiteCGSS/Views/Stack/LoggerStack.h"

#ifdef NDEBUG
#define CGSS_LOG_STACK nullptr
#else
#define CGSS_LOG_STACK "Stack.log"
#endif

cgss::LoggerStack& cgss::GetLoggerStack() {
	static LoggerStack logger{ CGSS_LOG_STACK };
	return logger;
}
