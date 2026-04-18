#include "RenderTarget.h"

namespace sf {
	class RenderTarget;
}

cgss::RenderTarget::RenderTarget(sf::RenderTarget& target) noexcept : m_target(&target) {}
