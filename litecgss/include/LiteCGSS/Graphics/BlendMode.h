#ifndef CGSS_BLEND_MODE_H
#define CGSS_BLEND_MODE_H

// Backend-agnostic blending enums. Numeric values are chosen to match
// sf::BlendMode::Factor / sf::BlendMode::Equation so that static_cast at the
// backend boundary is safe.
namespace cgss {

	enum class BlendFactor : int {
		Zero             = 0,
		One              = 1,
		SrcColor         = 2,
		OneMinusSrcColor = 3,
		DstColor         = 4,
		OneMinusDstColor = 5,
		SrcAlpha         = 6,
		OneMinusSrcAlpha = 7,
		DstAlpha         = 8,
		OneMinusDstAlpha = 9
	};

	enum class BlendEquation : int {
		Add             = 0,
		Subtract        = 1,
		ReverseSubtract = 2,
		Min             = 3,
		Max             = 4
	};

}

#endif
