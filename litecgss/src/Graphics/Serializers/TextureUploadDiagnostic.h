#ifndef CGSS_TEXTURE_UPLOAD_DIAGNOSTIC_H
#define CGSS_TEXTURE_UPLOAD_DIAGNOSTIC_H

#include <iostream>
#include <ostream>

namespace cgss::detail {

// Context attached to one upload attempt. Carries the GPU-side limits that
// the diagnostic message quotes back to the user (max size, NPOT support),
// plus the stream to write to. Pulled out of TextureSerializer so unit tests
// can drive the failure path without an OpenGL context: production callers
// fill it from Ops::texture_max_size() / texture_supports_npot(), tests
// pass in fabricated values and a std::ostringstream.
struct TextureUploadDiagnostics {
	std::ostream& err;
	unsigned int maxTextureSize;
	bool npotSupported;
};

// Drives the "create then upload" sequence, with a single high-level
// diagnostic on create-failure (the size limit a PSDK maker can actually
// act on — repack the bitmap). Templated on the create/update callables so
// the test can substitute lambdas that don't touch a graphics backend.
//
// Contract:
//   createFn(w, h) -> bool     true on success
//   updateFn(pixels, w, h)     called only when createFn returns true
//   returns 0 on success, static_cast<unsigned int>(-1) on failure.
template <typename CreateFn, typename UpdateFn>
inline unsigned int uploadDecodedPixels(
    CreateFn&& createFn,
    UpdateFn&& updateFn,
    const unsigned char* pixels,
    unsigned int w,
    unsigned int h,
    const TextureUploadDiagnostics& diag) {
	try {
		if (!createFn(w, h)) {
			// The backend may already print a low-level error when
			// GL_MAX_TEXTURE_SIZE is exceeded (after POT padding on
			// devices without NPOT support). Emit a higher-level message that names the actionable hint, so PSDK
			// makers get a clear "this asset is too large for the target
			// GPU" log instead of a silently-broken sprite downstream.
			diag.err << "Texture upload failed: source " << w << "x" << h
			         << " (GPU max " << diag.maxTextureSize << "x" << diag.maxTextureSize
			         << ", NPOT support: " << (diag.npotSupported ? "yes" : "no - padded to next POT")
			         << "). Consider repacking the bitmap (e.g. multi-row atlas)."
			         << std::endl;
			return static_cast<unsigned int>(-1);
		}
		updateFn(pixels, w, h);
		return 0;
	} catch (...) {
		return static_cast<unsigned int>(-1);
	}
}

// Decoded image -> GPU texture through the backend's Ops, with the
// diagnostic above when the backend refuses it (an oversized asset).
template <class Ops, class NativeTexture, class NativeImage>
inline bool uploadImage(NativeTexture& texture, const NativeImage& image) {
	const auto size = Ops::get_image_size(image);
	const TextureUploadDiagnostics diag{ std::cerr, Ops::texture_max_size(), Ops::texture_supports_npot() };
	return uploadDecodedPixels(
		[&](unsigned int, unsigned int) { return Ops::texture_create_from_image(texture, image); },
		[](const unsigned char*, unsigned int, unsigned int) {},
		nullptr, size.x, size.y, diag) == 0;
}

} // namespace cgss::detail

#endif
