#ifndef CGSS_QUATERNION_H
#define CGSS_QUATERNION_H

// Reserved public layout for unit-quaternion rotations (3D). Identity
// quaternion by default. Math operations (multiplication, normalize,
// slerp) will ship in a utility TU alongside the first 3D backend —
// this header reserves only the POD shape so the public layout is
// stable ahead of time.

namespace cgss {

	struct Quaternion {
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;
		float w = 1.f; // identity

		constexpr bool operator==(const Quaternion& o) const noexcept {
			return x == o.x && y == o.y && z == o.z && w == o.w;
		}
		constexpr bool operator!=(const Quaternion& o) const noexcept { return !(*this == o); }
	};

}

#endif
