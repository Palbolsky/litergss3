#include "LiteCGSS/Views/Stack/DrawableStack.h"
#include "Common/Profiling.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Graphics/Sprite.h"

void cgss::DrawableStack::draw(cgss::Camera& defview, cgss::RenderTarget& target) const  {
	CGSS_PROFILE_ZONE("cgss::DrawableStack::draw");
	using Ops = cgss::backend::ActiveBackend::Ops;
	// Apply the default view once. View-mutating items (ViewportItem,
	// FramedViewItem) save+restore the incoming view themselves, so it is
	// invariant across iterations — no per-element re-set needed.
	Ops::target_set_view(cgss::backend::native_of(target), cgss::backend::native_of(defview));
	for (auto& element: *this) {
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(cgss::RenderTarget& target) const {
	CGSS_PROFILE_ZONE("cgss::DrawableStack::drawFast");
	for (auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		sp->setOpacity(opacity);
	}
}
