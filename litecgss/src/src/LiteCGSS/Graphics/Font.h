#ifndef CGSS_FONT_H
#define CGSS_FONT_H

#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/FontFwd.h"

namespace cgss {

	// Templated wrapper over `Backend::native_font`. Consumers refer to it
	// as `cgss::Font`. Copies perform a deep copy of the underlying native
	// font (which itself owns font data — copies are relatively expensive).
	//
	// `raw()` is the temporary escape hatch used by implementation .cpps;
	// it will retire in PR L.
	template <class B>
	class basic_Font {
		friend struct cgss::backend::SFMLBackend::Ops;
	public:
		using native_type = typename B::native_font;

		basic_Font();
		basic_Font(const basic_Font& other);
		basic_Font(basic_Font&& other) noexcept;
		basic_Font& operator=(const basic_Font& other);
		basic_Font& operator=(basic_Font&& other) noexcept;
		~basic_Font();

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_font> m_font;
	};

}

#endif
