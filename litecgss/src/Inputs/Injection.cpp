#include "LiteCGSS/Inputs/Injection.h"
#include "LiteCGSS/Inputs/Keyboard.h"
#include "Inputs/InjectionQueue.h"

#include <deque>
#include <vector>
#include <algorithm>

namespace {
	struct Pending {
		cgss::KeyEvent key;
		bool down;
	};

	std::deque<Pending> g_pending;
	std::vector<cgss::KeyEvent> g_down;

	auto findDown(cgss::KeyCode key) {
		return std::find_if(g_down.begin(), g_down.end(), [key](const cgss::KeyEvent& e) { return e.code == key; });
	}
}

void cgss::input::injectKey(KeyCode key, Scancode scancode, bool down) {
	KeyEvent event;
	event.code = key;
	event.scancode = scancode != Scancode::Unknown ? scancode : keyboard::delocalize(key);
	const auto known = findDown(key);
	if (down) {
		if (known == g_down.end()) g_down.push_back(event);
	} else if (known != g_down.end()) {
		g_down.erase(known);
	}
	g_pending.push_back({ event, down });
}

void cgss::input::releaseInjectedKeys() {
	const auto down = g_down;
	for (const auto& event : down) injectKey(event.code, event.scancode, false);
}

bool cgss::input::isInjectedKeyDown(KeyCode key) {
	return findDown(key) != g_down.end();
}

bool cgss::input::isInjectedScancodeDown(Scancode scancode) {
	return std::any_of(g_down.begin(), g_down.end(), [scancode](const KeyEvent& e) { return e.scancode == scancode; });
}

bool cgss::input::detail::popInjectedKey(KeyEvent& key, bool& down) {
	if (g_pending.empty()) return false;
	key = g_pending.front().key;
	down = g_pending.front().down;
	g_pending.pop_front();
	return true;
}
