#ifndef CGSS_CAMERA_H
#define CGSS_CAMERA_H

#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/CameraFwd.h"

namespace cgss {

	// Templated wrapper over `Backend::native_view` (the backend's 2D
	// view/camera type — sf::View under SFMLBackend). Consumers refer to it
	// as `cgss::Camera`. Copies perform a deep copy of the underlying native
	// view (one heap allocation per clone).
	//
	// `raw()` is the temporary escape hatch used by implementation .cpps;
	// it will retire in PR L.
	template <class B>
	class basic_Camera {
		friend struct cgss::backend::SFMLBackend::Ops;
	public:
		using native_type = typename B::native_view;

		basic_Camera();
		basic_Camera(const basic_Camera& other);
		basic_Camera(basic_Camera&& other) noexcept;
		basic_Camera& operator=(const basic_Camera& other);
		basic_Camera& operator=(basic_Camera&& other) noexcept;
		~basic_Camera();

		explicit basic_Camera(const native_type& view);

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_view> m_view;
	};

}

#endif
