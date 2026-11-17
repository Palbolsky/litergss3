#include "Views/FrameDump.h"

#if defined(LITECGSS_FRAME_DUMP_ENABLED)

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "LiteCGSS/Image/Image.h"
#include "LiteCGSS/Image/Serializers/ImageSerializer.h"

namespace {
	struct Config {
		bool active = false;
		std::filesystem::path dir;
		unsigned long every = 0; // 0: on request only
	};

	const Config& config() {
		static const Config parsed = [] {
			Config c;
			const char* value = std::getenv("LITECGSS_DUMP_FRAMES");
			if (value == nullptr || *value == '\0') return c;
			std::string spec{ value };
			const auto option = spec.rfind(":every=");
			if (option != std::string::npos) {
				c.every = std::strtoul(spec.c_str() + option + 7, nullptr, 10);
				spec.erase(option);
			}
			c.dir = spec;
			std::error_code error;
			std::filesystem::create_directories(c.dir, error);
			c.active = !error;
			if (error) std::fprintf(stderr, "LiteCGSS: frame dump off, cannot create %s\n", spec.c_str());
			return c;
		}();
		return parsed;
	}
}

void cgss::detail::frameDumpAfterDraw(const void* window, double scale,
                                      const std::function<std::unique_ptr<cgss::Texture>()>& capture) {
	const Config& c = config();
	if (!c.active) return;
	static std::unordered_map<const void*, unsigned long> frames;
	const unsigned long frame = ++frames[window];

	const auto request = c.dir / "request";
	std::error_code error;
	const bool requested = std::filesystem::exists(request, error);
	if (!requested && (c.every == 0 || frame % c.every != 0)) return;

	auto texture = capture();
	if (!texture) return;
	cgss::Image image = texture->copyToImage();
	char name[96];
	std::snprintf(name, sizeof(name), "f%06lu-%ux%u-s%g.png", frame, image.width(), image.height(), scale);
	cgss::ImageFileSerializer saver{ (c.dir / name).string() };
	image.write(saver);
	if (requested) std::filesystem::remove(request, error);
}

#else

void cgss::detail::frameDumpAfterDraw(const void*, double, const std::function<std::unique_ptr<cgss::Texture>()>&) {}

#endif
