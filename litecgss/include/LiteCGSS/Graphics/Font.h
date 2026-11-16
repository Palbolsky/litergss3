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
		friend struct cgss::backend::ActiveBackend::Ops;
	public:
		using native_type = typename B::native_font;

		basic_Font();
		basic_Font(const basic_Font& other);
		basic_Font(basic_Font&& other) noexcept;
		basic_Font& operator=(const basic_Font& other);
		basic_Font& operator=(basic_Font&& other) noexcept;
		~basic_Font();

		// Load a font from a .ttf / .otf file. Returns true on success.
		// On failure, the font state is left unloaded (isLoaded() == false).
		bool load(const std::string& filename);
		// Same, but also rebuilds the GPU atlas at exactly `fontSize` pixels
		// per glyph. Required for pixel fonts under raylib — without this,
		// LoadFont's default base size (32) downscales tiny PSDK fonts and
		// produces aliased output. SFML ignores the size and rasterizes on
		// demand; raylib reloads via LoadFontEx.
		bool load(const std::string& filename, unsigned int fontSize);
		bool isLoaded() const noexcept { return m_loaded; }

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;

		std::unique_ptr<typename B::native_font> m_font;
		bool m_loaded = false;
	};

}

#endif
