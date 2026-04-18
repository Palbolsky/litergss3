#ifndef CGSS_CAMERA_H
#define CGSS_CAMERA_H

#include <memory>

namespace sf {
	class View;
}

namespace cgss {

	// Opaque wrapper around the backend 2D view/camera (currently sf::View).
	// Holds the sf::View via pimpl so that public LiteCGSS headers do not need
	// to include <SFML/Graphics/View.hpp>. Copies perform a deep copy of the
	// underlying sf::View (one heap allocation per clone).
	//
	// The raw() accessor is a temporary escape hatch used by implementation
	// .cpp files; it will disappear once the backend is replaced.
	class Camera {
	public:
		Camera();
		Camera(const Camera& other);
		Camera(Camera&& other) noexcept;
		Camera& operator=(const Camera& other);
		Camera& operator=(Camera&& other) noexcept;
		~Camera();

		explicit Camera(const sf::View& view);

		sf::View& raw() noexcept;
		const sf::View& raw() const noexcept;

	private:
		std::unique_ptr<sf::View> m_view;
	};

}

#endif
