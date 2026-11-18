#include "Common/Log.h"

#ifndef CGSS_NO_LOGS

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
	// (12:04:31)[I](  Viewport.cpp l.155) Viewport: View 0x5581d2c0 updated...
	// The level is colored on a terminal.
	constexpr auto LinePattern = "(%H:%M:%S)%^[%L]%$(%15!s l.%3#) %n: %v";

	// Every component shares them: one stream, lines never interleaved
	const std::vector<spdlog::sink_ptr>& Sinks() {
		static const auto sinks = [] {
			auto result = std::vector<spdlog::sink_ptr>{ std::make_shared<spdlog::sinks::stderr_color_sink_mt>() };
			// A file only where the host asks for one (agent runs give each run its own folder)
			const char* dir = std::getenv("LITECGSS_LOG_DIR");
			if (dir != nullptr && *dir != '\0') {
				auto path = std::string{ dir };
				if (path.back() != '/' && path.back() != '\\') path += '/';
				try {
					result.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path + "LiteCGSS.log", true));
				} catch (const spdlog::spdlog_ex&) {
					// An unwritable log folder costs the file only: stderr still has the lines
				}
			}
			for (auto& sink : result) {
				sink->set_pattern(LinePattern);
			}
			return result;
		}();
		return sinks;
	}

	// LITECGSS_LOG_LEVEL, as spdlog's SPDLOG_LEVEL: "level" for every component,
	// "Component=level" for one, comma-separated. Unknown levels are ignored.
	struct LevelOverrides {
		std::optional<spdlog::level::level_enum> all;
		std::unordered_map<std::string, spdlog::level::level_enum> byComponent;
	};

	LevelOverrides ParseLevelOverrides(const char* text) {
		auto overrides = LevelOverrides{};
		auto entries = std::istringstream{ text == nullptr ? "" : text };
		for (std::string entry; std::getline(entries, entry, ',');) {
			const auto separator = entry.find('=');
			auto component = separator == std::string::npos ? std::string{} : entry.substr(0, separator);
			auto levelName = separator == std::string::npos ? entry : entry.substr(separator + 1);
			std::transform(levelName.begin(), levelName.end(), levelName.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			const auto level = spdlog::level::from_str(levelName);
			if (level == spdlog::level::off && levelName != "off") {
				continue;
			}
			if (component.empty()) {
				overrides.all = level;
			} else {
				overrides.byComponent[component] = level;
			}
		}
		return overrides;
	}

	spdlog::level::level_enum LevelOf(const std::string& component, spdlog::level::level_enum defaultLevel) {
		static const auto overrides = ParseLevelOverrides(std::getenv("LITECGSS_LOG_LEVEL"));
		const auto it = overrides.byComponent.find(component);
		if (it != overrides.byComponent.end()) return it->second;
		return overrides.all.value_or(defaultLevel);
	}
}

std::shared_ptr<spdlog::logger> cgss::log::Create(const char* component, spdlog::level::level_enum defaultLevel) {
	const auto& sinks = Sinks();
	auto logger = std::make_shared<spdlog::logger>(component, sinks.begin(), sinks.end());
	logger->set_level(LevelOf(component, defaultLevel));
	// What leads up to a crash reaches the file
	logger->flush_on(spdlog::level::warn);
	return logger;
}

#endif
