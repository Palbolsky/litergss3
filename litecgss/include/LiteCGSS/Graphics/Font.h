#ifndef CGSS_FONT_H
#define CGSS_FONT_H

#include <memory>
#include <string>
#include <vector>

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

		// Load a font from a .ttf / .otf file, from the VFS when it holds it
		// (LITECGSS_USE_PHYSFS: a mounted game archive), else from the file
		// system. Returns true on success.
		// On failure, the font state is left unloaded (isLoaded() == false).
		bool load(const std::string& filename);
		// Same, but also builds the glyph atlas at exactly `fontSize` pixels
		// per glyph. Required for pixel fonts on backends that rasterize into
		// a fixed-size atlas — without this, a default base size downscales
		// tiny fonts and produces aliased output. Backends that rasterize on
		// demand ignore the size.
		bool load(const std::string& filename, unsigned int fontSize);
		bool isLoaded() const noexcept { return m_loaded; }

	private:
		native_type& native() noexcept;
		const native_type& native() const noexcept;
		bool loadFile(const std::string& filename, int fontSize);

		// The file's bytes when it came from the VFS: a backend may read them
		// as long as the font (and its copies) live. Declared first, freed last.
		std::shared_ptr<const std::vector<char>> m_data;
		std::unique_ptr<typename B::native_font> m_font;
		bool m_loaded = false;
	};

}

#endif
