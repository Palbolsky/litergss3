#ifndef LR_DISPLAY_WINDOW_H
#define LR_DISPLAY_WINDOW_H

#include "View.h"
#include "ViewAuthorizations.h"

namespace lr {
	class DisplayWindow :
		public View {

	};
	
	class Sprite;
	class Text;
	class Viewport;

	template <>
	struct ViewAuthorizations<DisplayWindow> : public AuthorizedDrawableSet<Sprite, Text, Viewport> {};
}

#endif