// Raylib implementation of cgss::ViewportItem::takeSnapshot. The shared bulk
// of Viewport's body lives in Views/Viewport.cpp; this one method is split
// out per-backend because it's render-target-shaped (SFML uses
// sf::RenderTexture + sf::Sprite for the off-screen render-and-grab; raylib
// uses BeginTextureMode + LoadImageFromTexture for the same dance).
//
// Y-flip note: raylib FBOs are bottom-up in texel space, so after reading
// pixels we ImageFlipVertical before promoting to a fresh GL texture —
// otherwise the snapshot would render upside down when bound to a Sprite.

#include <raylib.h>
#include "Common/Profiling.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/Viewport.h"

std::unique_ptr<cgss::Texture> cgss::ViewportItem::takeSnapshot() const {
	CGSS_PROFILE_ZONE("cgss::ViewportItem::takeSnapshot");
	using Ops = cgss::backend::ActiveBackend::Ops;
	using cgss::backend::native_of;

	const unsigned int w = static_cast<unsigned int>(ViewBox::getWidth());
	const unsigned int h = static_cast<unsigned int>(ViewBox::getHeight());
	if (w == 0 || h == 0) return nullptr;
	if (!IsWindowReady()) return nullptr;

	::RenderTexture rt{};
	if (!Ops::render_texture_create(rt, w, h)) return nullptr;

	cgss::Camera renderView = ViewBox::clone();
	Ops::view_set_viewport(native_of(renderView), 0.f, 0.f, 1.f, 1.f);

	// raylib drawing is implicit on whichever Begin* mode is active;
	// render_texture_clear_color opens the BeginTextureMode block. The
	// RenderTarget passed to drawables() is a thin wrapper — its native
	// reference is unused by raylib's draw_* Ops (target arg is ignored
	// — see RaylibBackend.h:1396).
	cgss::RenderTarget renderTarget{rt};
	Ops::render_texture_clear_color(rt, cgss::Color{ 0, 0, 0, 0 });
	Ops::render_texture_set_view(rt, native_of(renderView));
	drawables().drawFast(renderTarget);
	Ops::render_texture_display(rt);

	// Read the FBO pixels back into a CPU image, flip Y, and upload to a
	// fresh standalone texture. We can't return rt.texture directly — the
	// RenderTexture owns the GL handle and unloading it would invalidate
	// the Sprite that PSDK binds the snapshot to.
	::Image cpuImage = LoadImageFromTexture(rt.texture);
	UnloadRenderTexture(rt);
	if (cpuImage.data == nullptr) return nullptr;
	ImageFlipVertical(&cpuImage);
	::Texture2D outTex = LoadTextureFromImage(cpuImage);
	UnloadImage(cpuImage);
	if (outTex.id == 0) return nullptr;

	return std::make_unique<cgss::Texture>(cgss::Texture::create_from_native(outTex));
}
