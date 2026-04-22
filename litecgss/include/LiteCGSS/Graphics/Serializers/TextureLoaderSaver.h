#ifndef CGSS_TEXTURE_LOADER_SAVER_H
#define CGSS_TEXTURE_LOADER_SAVER_H

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Loader.h"
#include "LiteCGSS/Common/Saver.h"

namespace cgss {

	// Backend-parameterised aliases. Consumers refer to `TextureLoader` /
	// `TextureSaver` (instantiated for the active backend); the underlying
	// `Loader<>` / `Saver<>` templates stay generic.
	template <class B>
	using basic_TextureLoader = Loader<typename B::native_texture, typename B::native_image&>;

	template <class B>
	using basic_TextureSaver = Saver<typename B::native_texture>;

	using TextureLoader = basic_TextureLoader<cgss::backend::ActiveBackend>;
	using TextureSaver  = basic_TextureSaver<cgss::backend::ActiveBackend>;
}

#endif
