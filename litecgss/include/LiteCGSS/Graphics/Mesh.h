#ifndef CGSS_MESH_H
#define CGSS_MESH_H

// Placeholder for a triangular-mesh primitive wrapper. Reserved public
// layout ahead of the first 3D backend. Gated on the
// `supports_mesh_primitive` capability tag — a basic_Mesh<B> will only
// compile under a backend that flips that on.
//
// No implementation yet. Including this header under a 2D-only backend
// is harmless (exposes only a forward declaration).

namespace cgss {

	template <class B>
	class basic_Mesh; // definition comes with the first 3D backend

}

#endif
