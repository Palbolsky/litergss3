// Raylib stub for `cgss::ViewportItem::takeSnapshot`. Returns nullptr —
// a real raylib impl would use BeginTextureMode / EndTextureMode to render
// the viewport contents into a RenderTexture, then LoadImageFromTexture
// to grab pixels into a cgss::Texture.

#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/Viewport.h"

std::unique_ptr<cgss::Texture> cgss::ViewportItem::takeSnapshot() const {
	return nullptr;
}
