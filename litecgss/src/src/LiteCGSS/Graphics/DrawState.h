#ifndef CGSS_DRAW_STATE_H
#define CGSS_DRAW_STATE_H

#include <memory>

namespace sf {
	class RenderStates;
}

namespace cgss {

	// Opaque wrapper around the backend draw state (currently sf::RenderStates —
	// blend mode + transform + texture + shader bundle). Uses pimpl so that
	// public LiteCGSS headers do not need to include <SFML/Graphics/RenderStates.hpp>.
	// Copies perform a deep copy of the underlying sf::RenderStates (one heap
	// allocation per clone).
	//
	// The raw() accessor is a temporary escape hatch used by implementation
	// .cpp files; it will disappear once the backend is replaced and draw
	// operations are wrapped natively.
	class DrawState {
	public:
		DrawState();
		DrawState(const DrawState& other);
		DrawState(DrawState&& other) noexcept;
		DrawState& operator=(const DrawState& other);
		DrawState& operator=(DrawState&& other) noexcept;
		~DrawState();

		explicit DrawState(const sf::RenderStates& raw);

		sf::RenderStates& raw() noexcept;
		const sf::RenderStates& raw() const noexcept;

	private:
		std::unique_ptr<sf::RenderStates> m_raw;
	};

}

#endif
