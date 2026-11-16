#include "Backend/Internal/RaylibWindowStack.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace cgss::backend::raylib_detail {

	namespace {
		// Windows are opened and closed from the threads driving them
		// (PSDK opens its error window from a thread of its own).
		std::mutex g_mutex;
		std::vector<raylib::Window*> g_stack; // last: the screen's owner
	}

	void windowOpened(raylib::Window& window) {
		std::lock_guard<std::mutex> lock(g_mutex);
		g_stack.erase(std::remove(g_stack.begin(), g_stack.end(), &window), g_stack.end());
		g_stack.push_back(&window);
	}

	raylib::Window* windowClosed(raylib::Window& window) {
		std::lock_guard<std::mutex> lock(g_mutex);
		g_stack.erase(std::remove(g_stack.begin(), g_stack.end(), &window), g_stack.end());
		return g_stack.empty() ? nullptr : g_stack.back();
	}

	raylib::Window* screenOwner() {
		std::lock_guard<std::mutex> lock(g_mutex);
		return g_stack.empty() ? nullptr : g_stack.back();
	}

}
