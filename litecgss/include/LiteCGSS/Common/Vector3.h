#ifndef CGSS_VECTOR3_H
#define CGSS_VECTOR3_H

// Reserved public layout for a 3D float vector. Currently a plain POD;
// 3D-capable backends (e.g. the planned bgfx backend) will use this at
// the API boundary of mesh / transform / camera ops. Kept backend-agnostic
// (no math operators beyond equality) so the header stays free of
// dependencies. Math operations live in a future non-header utility TU.

namespace cgss {

	struct Vector3 {
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;

		constexpr bool operator==(const Vector3& o) const noexcept {
			return x == o.x && y == o.y && z == o.z;
		}
		constexpr bool operator!=(const Vector3& o) const noexcept { return !(*this == o); }
	};

}

#endif
