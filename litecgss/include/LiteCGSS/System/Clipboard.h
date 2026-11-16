#ifndef CGSS_SYSTEM_CLIPBOARD_H
#define CGSS_SYSTEM_CLIPBOARD_H

// System clipboard, UTF-8. Backend-neutral: implemented over the active
// backend's Ops in src/System/.

#include <string>

namespace cgss::clipboard {

	// Empty when the clipboard is empty or holds no text.
	std::string getString();
	void setString(const std::string& utf8);

}

#endif
