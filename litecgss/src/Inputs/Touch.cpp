#include "LiteCGSS/Inputs/Touch.h"

#include "Backend/ActiveBackend.h"

using Backend = cgss::backend::ActiveBackend;
using Ops     = Backend::Ops;

bool cgss::touch::isSupported() noexcept {
	return Backend::has_input_touch;
}

unsigned int cgss::touch::count() {
	return Ops::touch_count();
}

cgss::backend::Point2i cgss::touch::position(unsigned int index) {
	return Ops::touch_position(index);
}

unsigned int cgss::touch::id(unsigned int index) {
	return Ops::touch_id(index);
}
