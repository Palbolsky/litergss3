#ifndef CGSS_INPUTS_TOUCH_H
#define CGSS_INPUTS_TOUCH_H

// Polled touch state. Backend-neutral (see Inputs/Keyboard.h).

#include "LiteCGSS/Backend/BackendCommon.h"

namespace cgss::touch {

	// False when the active backend has no touch polling (e.g. SFML): every
	// query below then reports no touch point.
	bool isSupported() noexcept;

	unsigned int count();
	backend::Point2i position(unsigned int index);
	unsigned int id(unsigned int index);

}

#endif
