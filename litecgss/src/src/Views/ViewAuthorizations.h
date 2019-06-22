#ifndef LR_VIEW_AUTHORIZATIONS_H
#define LR_VIEW_AUTHORIZATIONS_H
#include <type_traits>

namespace lr {
	template <class Drawable>
	struct AuthorizedDrawableBox {};

	template <class ... Drawable>
	struct AuthorizedDrawableSet : public AuthorizedDrawableBox<Drawable>... {};

	template <class View>
	struct ViewAuthorizations {};

	template <class Drawable, class View>
	struct DrawableIsAuthorizedOnView : public std::is_base_of<AuthorizedDrawableBox<Drawable>, ViewAuthorizations<View>> {};
}

#endif