#include "Backend/Internal/RaylibPlatformQueries.h"

#include <raylib.h>

#if defined(__ELF__)
#  define CGSS_GLFW_WEAK __attribute__((weak))
#  define CGSS_HAS_GLFW_QUERIES 1
#else
#  define CGSS_HAS_GLFW_QUERIES 0
#endif

#if CGSS_HAS_GLFW_QUERIES
extern "C" {
	struct GLFWmonitor;
	struct GLFWvidmode {
		int width;
		int height;
		int redBits;
		int greenBits;
		int blueBits;
		int refreshRate;
	};
	GLFWmonitor* glfwGetPrimaryMonitor(void) CGSS_GLFW_WEAK;
	const GLFWvidmode* glfwGetVideoModes(GLFWmonitor* monitor, int* count) CGSS_GLFW_WEAK;
	void* glfwGetProcAddress(const char* procname) CGSS_GLFW_WEAK;
	struct GLFWwindow;
	void glfwMakeContextCurrent(GLFWwindow* window) CGSS_GLFW_WEAK;
}
#endif

namespace cgss::backend::raylib_detail {

	namespace {
		// OpenGL 3.3's guaranteed minimum for GL_MAX_TEXTURE_SIZE.
		constexpr unsigned int kMinimumMaxTextureSize = 1024;
		constexpr unsigned int kGlMaxTextureSize = 0x0D33;
	}

	std::vector<cgss::VideoMode> fullscreenModes() {
		std::vector<cgss::VideoMode> modes;
#if CGSS_HAS_GLFW_QUERIES
		if (glfwGetPrimaryMonitor != nullptr && glfwGetVideoModes != nullptr && IsWindowReady()) {
			int count = 0;
			const GLFWvidmode* list = glfwGetVideoModes(glfwGetPrimaryMonitor(), &count);
			// GLFW lists modes ascending; SFML (LiteRGSS2) lists them best first.
			for (int i = count - 1; list != nullptr && i >= 0; --i) {
				const auto& m = list[i];
				const unsigned int bpp = static_cast<unsigned int>(m.redBits + m.greenBits + m.blueBits);
				const cgss::VideoMode mode{ static_cast<unsigned int>(m.width), static_cast<unsigned int>(m.height),
				                            bpp == 24 ? 32u : bpp };
				bool duplicate = false; // same size at several refresh rates
				for (const auto& known : modes) {
					duplicate = duplicate || (known.width == mode.width && known.height == mode.height && known.bitsPerPixel == mode.bitsPerPixel);
				}
				if (!duplicate) modes.push_back(mode);
			}
		}
#endif
		if (modes.empty() && IsWindowReady()) {
			const int monitor = GetCurrentMonitor();
			modes.push_back({ static_cast<unsigned int>(GetMonitorWidth(monitor)),
			                  static_cast<unsigned int>(GetMonitorHeight(monitor)), 32u });
		}
		return modes;
	}

	unsigned int queriedMaxTextureSize() {
#if CGSS_HAS_GLFW_QUERIES
		// One GL context per process: the first successful query holds.
		static unsigned int cached = 0;
		if (cached == 0 && glfwGetProcAddress != nullptr && IsWindowReady()) {
			using GetIntegerv = void (*)(unsigned int, int*);
			if (auto fn = reinterpret_cast<GetIntegerv>(glfwGetProcAddress("glGetIntegerv"))) {
				int size = 0;
				fn(kGlMaxTextureSize, &size);
				if (size > 0) cached = static_cast<unsigned int>(size);
			}
		}
		return cached;
#else
		return 0;
#endif
	}

	unsigned int maxTextureSize() {
		const auto size = queriedMaxTextureSize();
		return size != 0 ? size : kMinimumMaxTextureSize;
	}

	bool makeContextCurrent(bool current) {
#if CGSS_HAS_GLFW_QUERIES
		if (glfwMakeContextCurrent != nullptr && IsWindowReady()) {
			glfwMakeContextCurrent(current ? static_cast<GLFWwindow*>(GetWindowHandle()) : nullptr);
			return true;
		}
#endif
		(void)current;
		return false;
	}

}
