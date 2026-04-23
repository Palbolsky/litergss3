#ifndef CGSS_CAMERA3D_H
#define CGSS_CAMERA3D_H

// Placeholder for a perspective / 3D camera wrapper. Reserved so downstream
// consumers can depend on the header path before the full implementation
// lands. Gated on the `supports_3d` and `supports_perspective_camera`
// capability tags — a basic_Camera3D<B> will only compile under a backend
// that flips those on (bgfx, future SDL3-GPU, etc.).
//
// No implementation yet. Including this header under a 2D-only backend
// is harmless (it exposes only a forward declaration).

namespace cgss {

	template <class B>
	class basic_Camera3D; // definition comes with the first 3D backend

}

#endif
