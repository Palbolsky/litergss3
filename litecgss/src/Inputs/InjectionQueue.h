#ifndef CGSS_INPUTS_INJECTION_QUEUE_H
#define CGSS_INPUTS_INJECTION_QUEUE_H

// Engine-internal side of cgss::input::injectKey: DisplayWindow::popEvent
// drains the injected key events before the device's.

#include "LiteCGSS/Events/EventPayloads.h"

namespace cgss::input::detail {

	bool popInjectedKey(KeyEvent& key, bool& down);

}

#endif
