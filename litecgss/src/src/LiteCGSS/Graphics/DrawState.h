#ifndef CGSS_DRAW_STATE_H
#define CGSS_DRAW_STATE_H

#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/DrawStateFwd.h"

namespace cgss {

	// Templated wrapper over `Backend::native_render_states` — the bundle of
	// blend mode + transform + texture + shader passed alongside each draw.
	// Consumers refer to it as `cgss::DrawState`. Copies perform a deep
	// copy of the underlying native render states (one heap allocation
	// per clone).
	//
	// `raw()` is the temporary escape hatch used by implementation .cpps;
	// it will retire in PR L.
	template <class B>
	class basic_DrawState {
	public:
		using native_type = typename B::native_render_states;

		basic_DrawState();
		basic_DrawState(const basic_DrawState& other);
		basic_DrawState(basic_DrawState&& other) noexcept;
		basic_DrawState& operator=(const basic_DrawState& other);
		basic_DrawState& operator=(basic_DrawState&& other) noexcept;
		~basic_DrawState();

		explicit basic_DrawState(const native_type& raw);

		native_type& raw() noexcept;
		const native_type& raw() const noexcept;

	private:
		std::unique_ptr<typename B::native_render_states> m_raw;
	};

}

#endif
