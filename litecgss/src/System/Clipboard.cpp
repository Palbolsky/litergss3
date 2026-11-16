#include "LiteCGSS/System/Clipboard.h"

#include "Backend/ActiveBackend.h"

using Ops = cgss::backend::ActiveBackend::Ops;

std::string cgss::clipboard::getString() {
	return Ops::clipboard_get_string();
}

void cgss::clipboard::setString(const std::string& utf8) {
	Ops::clipboard_set_string(utf8);
}
