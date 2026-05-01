// Raylib body for `cgss::detail::backendCaptureScreen` — the hook
// StubDisplayWindowSnapshot::takeSnapshot delegates to. Under raylib,
// basic_DisplayWindow<B>::Snapshot resolves to StubDisplayWindowSnapshot
// (gated by RaylibBackend::has_snapshot = false), so this is the path
// PSDK's `Graphics.snap_to_bitmap` reaches when CGSS_BACKEND=raylib.
//
// The full SFML-side `freeze`/`transition` machinery is not provided —
// only a single-shot screen→texture capture, which covers the vast majority
// of snapshot usage (saved-game thumbnails, scene transition fades).

#include <raylib.h>
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/DisplayWindowSnapshot.h"

std::unique_ptr<cgss::Texture> cgss::detail::backendCaptureScreen() {
	if (!IsWindowReady()) return nullptr;
	::Image cpuImage = LoadImageFromScreen();
	if (cpuImage.data == nullptr) return nullptr;
	// LoadImageFromScreen returns top-down already (raylib does the FBO
	// Y-flip when reading the default framebuffer), so no ImageFlipVertical
	// here — unlike ViewportTakeSnapshot which reads from a render-texture.
	::Texture2D outTex = LoadTextureFromImage(cpuImage);
	UnloadImage(cpuImage);
	if (outTex.id == 0) return nullptr;
	return std::make_unique<cgss::Texture>(cgss::Texture::create_from_native(outTex));
}
