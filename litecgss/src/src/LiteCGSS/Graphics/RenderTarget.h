#ifndef CGSS_RENDER_TARGET_H
#define CGSS_RENDER_TARGET_H

namespace sf {
	class RenderTarget;
}

namespace cgss {

	// Opaque wrapper around the backend render target (currently sf::RenderTarget).
	// Public LiteCGSS API uses this type instead of the SFML one so that the
	// backend can be swapped without changing consumer-visible signatures.
	//
	// The raw() accessor is a temporary escape hatch: implementation .cpp files
	// still call SFML directly via raw(). It will disappear once the backend is
	// replaced and the rendering operations are wrapped natively.
	class RenderTarget {
	public:
		explicit RenderTarget(sf::RenderTarget& target) noexcept;

		RenderTarget(const RenderTarget&) = default;
		RenderTarget& operator=(const RenderTarget&) = default;
		RenderTarget(RenderTarget&&) noexcept = default;
		RenderTarget& operator=(RenderTarget&&) noexcept = default;

		sf::RenderTarget& raw() const noexcept { return *m_target; }

	private:
		sf::RenderTarget* m_target;
	};

}

#endif
