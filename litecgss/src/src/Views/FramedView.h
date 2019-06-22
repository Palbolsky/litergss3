#ifndef LR_FRAMED_VIEW_H
#define LR_FRAMED_VIEW_H

#include "View.h"
#include "ViewAuthorizations.h"

namespace lr {
	class FramedView :
		public View {

	};

	class Sprite;
	class Text;
	class Shape;

	template <>
	struct ViewAuthorizations<FramedView> : public AuthorizedDrawableSet<Sprite, Text, Shape> {};
}

#endif