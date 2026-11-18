#ifndef CGSS_LOG_H
#define CGSS_LOG_H

// Engine logging, on spdlog (header-only, vendored in external/spdlog).
// Private to src/: no public header includes it, so spdlog is neither part
// of LiteCGSS' API nor installed with it.
//
// A component owns a logger named after it, declared once in its .cpp with
// the level it logs from by default:
//
//   CGSS_LOG_COMPONENT("Viewport", info)
//   ...
//   CGSS_LOG_INFO("View {} resized to {}x{}", static_cast<void*>(this), w, h);
//
// Lines go to stderr. The host chooses the rest, in the environment:
//   LITECGSS_LOG_DIR    also write them to <dir>/LiteCGSS.log (no file otherwise)
//   LITECGSS_LOG_LEVEL  override the levels, with spdlog's syntax: "debug" for
//                       every component, "Viewport=debug,ZContainer=info" for
//                       some, both combined ("warn,Viewport=debug")
// CGSS_NO_LOGS compiles every call out, spdlog included.

#ifdef CGSS_NO_LOGS

namespace cgss::log {
	// Never called: it only keeps a call's arguments used, unevaluated
	template <class... Args>
	constexpr void Ignore(const Args&...) {}
}

#define CGSS_LOG_COMPONENT(component, lvl)
#define CGSS_LOG_DEBUG(...) (false ? cgss::log::Ignore(__VA_ARGS__) : void())
#define CGSS_LOG_INFO(...)  (false ? cgss::log::Ignore(__VA_ARGS__) : void())
#define CGSS_LOG_WARN(...)  (false ? cgss::log::Ignore(__VA_ARGS__) : void())
#define CGSS_LOG_ERROR(...) (false ? cgss::log::Ignore(__VA_ARGS__) : void())

#else

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <memory>
#include <spdlog/spdlog.h>

namespace cgss::log {
	// A logger named `component`, from `defaultLevel` up unless LITECGSS_LOG_LEVEL says otherwise
	std::shared_ptr<spdlog::logger> Create(const char* component, spdlog::level::level_enum defaultLevel);
}

#define CGSS_LOG_COMPONENT(component, lvl) \
	namespace { \
		[[maybe_unused]] spdlog::logger* CgssComponentLogger() { \
			static const auto logger = cgss::log::Create(component, spdlog::level::lvl); \
			return logger.get(); \
		} \
	}

#define CGSS_LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(CgssComponentLogger(), __VA_ARGS__)
#define CGSS_LOG_INFO(...)  SPDLOG_LOGGER_INFO(CgssComponentLogger(), __VA_ARGS__)
#define CGSS_LOG_WARN(...)  SPDLOG_LOGGER_WARN(CgssComponentLogger(), __VA_ARGS__)
#define CGSS_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(CgssComponentLogger(), __VA_ARGS__)

#endif

#endif
