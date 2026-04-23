#ifndef CGSS_TRANSFORM3D_H
#define CGSS_TRANSFORM3D_H

// Reserved public layout for a 3D TRS transform (position + rotation +
// scale). Distinct from cgss::Transformable (2D: x/y/origin/angle/scaleXY)
// — the 2D variant stays the canonical sprite/shape transform; this one
// is the 3D counterpart used by basic_Mesh and basic_Camera3D once they
// exist.
//
// Populated with identity defaults so default-construction yields a
// no-op transform. Matrix composition utilities will ship with the
// first 3D backend.

#include "LiteCGSS/Common/Quaternion.h"
#include "LiteCGSS/Common/Vector3.h"

namespace cgss {

	struct Transform3D {
		Vector3    position{ 0.f, 0.f, 0.f };
		Quaternion rotation{};                // identity
		Vector3    scale   { 1.f, 1.f, 1.f };
	};

}

#endif
