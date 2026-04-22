#ifndef CGSS_RENDER_TARGET_H
#define CGSS_RENDER_TARGET_H

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/RenderTargetFwd.h"

namespace cgss {

	// Templated wrapper over a reference to `Backend::native_render_target`.
	// Non-owning (just a pointer); freely copyable. Consumers refer to it as
	// `cgss::RenderTarget`.
	//
	// `raw()` is the temporary escape hatch used by implementation .cpps;
	// it will retire in PR L (replaced by `Backend::Ops::draw_*` direct calls).
	template <class B>
	class basic_RenderTarget {
		friend struct cgss::backend::ActiveBackend::Ops;
	public:
		using native_type = typename B::native_render_target;

		explicit basic_RenderTarget(native_type& target) noexcept;

		basic_RenderTarget(const basic_RenderTarget&) noexcept;
		basic_RenderTarget& operator=(const basic_RenderTarget&) noexcept;
		basic_RenderTarget(basic_RenderTarget&&) noexcept;
		basic_RenderTarget& operator=(basic_RenderTarget&&) noexcept;

	private:
		native_type& native() const noexcept;

		native_type* m_target;
	};

}

#endif
