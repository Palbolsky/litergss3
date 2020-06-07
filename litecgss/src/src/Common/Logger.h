#ifndef CGSS_LOGGER_H
#define CGSS_LOGGER_H
#include <fstream>
#include <iostream>
#include <Logging/MultiLogger.h>
#include <Logging/LogAsync.h>
#include <Logging/Logger.h>

namespace cgss {
    template <ska::LogLevel BaseLogLevel>
    using LoggerBaseType = ska::MultiLogger<
        ska::Logger<BaseLogLevel, ska::LogLevel::Error, ska::LogAsync>,
        ska::Logger<BaseLogLevel, ska::LogLevel::Error, ska::LogSync>
    >;

    template <ska::LogLevel BaseLogLevel>
    struct Logger : 
        public LoggerBaseType<BaseLogLevel> {
        Logger(const char* filename) {
            if (filename != nullptr) {
                m_fileOutput.open(filename);
                updatePatterns(LoggerBaseType<BaseLogLevel>::template get<0>());
                LoggerBaseType<BaseLogLevel>::template get<0>().addOutputTarget(m_fileOutput);
            }
            LoggerBaseType<BaseLogLevel>::template get<1>().addOutputTarget(std::cout);
            updatePatterns(LoggerBaseType<BaseLogLevel>::template get<1>());
        }

        virtual ~Logger() {
            LoggerBaseType<BaseLogLevel>::terminate();
        }
        
    private:
        template<class T>
		void updatePatterns(T& logger) {
			logger.setPattern(ska::LogLevel::Debug, "(%h:%m:%s)%10c[D]%8c(%15F l.%3l) %07c%v");
			logger.setPattern(ska::LogLevel::Info, "(%h:%m:%s)%11c[I]%8c(%15F l.%3l) %07c%v");
			logger.setPattern(ska::LogLevel::Warn, "(%h:%m:%s)%14c[W]%8c(%15F l.%3l) %07c%v");
			logger.setPattern(ska::LogLevel::Error, "(%h:%m:%s)%12c[E]%8c(%15F l.%3l) %07c%v");
			logger.enableComplexLogging();
		}

        std::ofstream m_fileOutput;
    };
}

#endif