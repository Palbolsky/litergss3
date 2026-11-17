#ifndef CGSS_LOGGER_H
#define CGSS_LOGGER_H
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <Logging/MultiLogger.h>
#include <Logging/LogAsync.h>
#include <Logging/Logger.h>

namespace cgss {
	// Where a log file named `filename` is written: in LITECGSS_LOG_DIR when
	// set (agent runs give each run its own), else in the working directory.
	inline std::string LogFilePath(const char* filename) {
		const char* dir = std::getenv("LITECGSS_LOG_DIR");
		if (dir == nullptr || *dir == '\0') return filename;
		std::string path{ dir };
		if (path.back() != '/' && path.back() != '\\') path += '/';
		return path + filename;
	}

	template <ska::LogLevel BaseLogLevel>
#ifdef CGSS_NO_LOGS
	using LoggerBaseType = ska::Logger<ska::LogLevel::Disabled, ska::LogLevel::Disabled, ska::LogSync>;
#else
	using LoggerBaseType = ska::Logger<BaseLogLevel, ska::LogLevel::Error, ska::LogSync>;
#endif

	template <ska::LogLevel BaseLogLevel>
	struct Logger :
		public LoggerBaseType<BaseLogLevel> {
		Logger(const char* filename) {
			updatePatterns(*this);
			LoggerBaseType<BaseLogLevel>::addOutputTarget(std::cout);
			if (filename != nullptr) {
				m_fileOutput.open(LogFilePath(filename));
				LoggerBaseType<BaseLogLevel>::addOutputTarget(m_fileOutput);
			}
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