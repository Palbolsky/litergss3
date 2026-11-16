#ifndef CGSS_SPRITE_FWD_H
#define CGSS_SPRITE_FWD_H

// Forward-declares `cgss::Sprite` and `cgss::SpriteItem` (= aliases of the
// templated basic_Sprite/basic_SpriteItem<ActiveBackend>) for other public
// headers that previously did `class Sprite;`.

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

namespace cgss {
	template <class B> class basic_Sprite;
	template <class B> class basic_SpriteItem;
	using Sprite = basic_Sprite<cgss::backend::ActiveBackend>;
	using SpriteItem = basic_SpriteItem<cgss::backend::ActiveBackend>;
}

#endif
