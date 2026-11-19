// Raylib implementation of cgss::ViewportItem::takeSnapshot. The shared bulk
// of Viewport's body lives in Views/Viewport.cpp; this one method is split
// out per-backend because it's render-target-shaped (SFML uses
// sf::RenderTexture + sf::Sprite for the off-screen render-and-grab; raylib
// uses BeginTextureMode for the same dance).
//
// The snapshot never leaves the GPU, as on SFML (its texture copy is an FBO
// blit). Reading it back to flip it on the CPU and uploading it again stalled
// on the GPU: PSDK's Graphics.snap_to_bitmap takes one snapshot per viewport,
// a battle start took ~1 s of them on the Android emulator.
//
// Y-flip note: raylib FBOs are bottom-up in texel space, a texture made from
// an image top-down. The last pass draws the render into a second render
// texture upside down, which stores it top-down; that texture outlives its
// framebuffer and is the snapshot.

#include <raylib.h>
#include <rlgl.h>
#include "Common/Profiling.h"
#include "Views/GLContext.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/Viewport.h"

std::unique_ptr<cgss::Texture> cgss::ViewportItem::takeSnapshot() const {
	cgss::detail::ensureGLContext();
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

	// The viewport's render states (its shader: PSDK's map tone) apply to
	// the snapshot too, as in the SFML implementation: the drawables'
	// result is drawn once more, through them, into a second texture.
	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates != nullptr) {
		::RenderTexture shaded{};
		if (Ops::render_texture_create(shaded, w, h)) {
			cgss::RenderTarget shadedTarget{shaded};
			Ops::render_texture_clear_color(shaded, cgss::Color{ 0, 0, 0, 0 });
			Ops::render_texture_set_view(shaded, native_of(renderView));
			cgss::backend::ActiveBackend::native_sprite drawn{};
			drawn.texture = &rt.texture;
			drawn.textureRect = cgss::IntRect{ 0, 0, static_cast<int>(w), static_cast<int>(h) };
			// draw_sprite with render states reads an FBO-backed sprite bottom-up
			Ops::draw_sprite(native_of(shadedTarget), drawn, native_of(*renderStates));
			Ops::render_texture_display(shaded);
			UnloadRenderTexture(rt);
			rt = shaded;
		}
	}

	// Upside down into a fresh render texture, as is: no blending, the
	// snapshot keeps the render's alpha. FBO texels are bottom-up, so drawing
	// the source rows as they are stores them top-down.
	::RenderTexture flipped{};
	if (!Ops::render_texture_create(flipped, w, h)) {
		UnloadRenderTexture(rt);
		return nullptr;
	}
	Ops::render_texture_clear_color(flipped, cgss::Color{ 0, 0, 0, 0 });
	rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
	BeginBlendMode(BLEND_CUSTOM);
	const float fw = static_cast<float>(w);
	const float fh = static_cast<float>(h);
	DrawTexturePro(rt.texture, ::Rectangle{ 0.f, 0.f, fw, fh }, ::Rectangle{ 0.f, 0.f, fw, fh },
		::Vector2{ 0.f, 0.f }, 0.f, ::WHITE);
	EndBlendMode();
	Ops::render_texture_display(flipped);
	UnloadRenderTexture(rt);

	// The texture stays, its framebuffer (and depth buffer) go: rlUnloadFramebuffer
	// deletes those only, UnloadRenderTexture would delete the texture too.
	::Texture2D outTex = flipped.texture;
	rlUnloadFramebuffer(flipped.id);
	if (outTex.id == 0) return nullptr;

	return std::make_unique<cgss::Texture>(cgss::Texture::create_from_native(outTex));
}
