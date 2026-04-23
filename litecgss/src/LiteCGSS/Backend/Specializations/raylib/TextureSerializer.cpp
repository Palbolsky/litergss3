// Raylib implementations of the Texture serializer family. The CPU-side
// ::Image is populated first via LoadImage / LoadImageFromMemory /
// GenImageColor; the GPU-side ::Texture is then uploaded via
// LoadTextureFromImage. The cgss::basic_Texture caller keeps both alive so
// later pixel queries and texture updates work symmetrically with SFML.
//
// Note: LoadTextureFromImage requires an active OpenGL context, so the
// window must have been created (InitWindow) before loading a texture.
// This matches SFML (sf::Texture::loadFromMemory also needs a valid GL
// context on some drivers); we don't gate on IsWindowReady here because
// a caller using textures pre-window is already in undefined territory.

#include "LiteCGSS/Backend/RaylibBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"

namespace cgss {

namespace {
	// Shared teardown: both Image and Texture may already hold raylib
	// allocations from a previous load — free them before overwriting.
	inline void unloadTextureAndImage(::Texture& texture, ::Image& image) {
		if (texture.id != 0) UnloadTexture(texture);
		if (image.data != nullptr) UnloadImage(image);
	}
}

template <>
bool basic_TextureMemorySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture& texture,
		const MemorySerializerData& rawData,
		cgss::backend::RaylibBackend::native_image& image) {
	unloadTextureAndImage(texture, image);
	// ".png" is a hint; raylib's stb_image-based decoder autodetects PNG
	// / JPG / etc. from the buffer header regardless of the extension.
	image = LoadImageFromMemory(".png",
	                            reinterpret_cast<const unsigned char*>(rawData.first),
	                            static_cast<int>(rawData.second));
	if (image.data == nullptr) return false;
	texture = LoadTextureFromImage(image);
	return texture.id != 0;
}

template <>
MemorySerializerData basic_TextureMemorySerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture& texture) {
	if (texture.id == 0) return { nullptr, 0u };
	::Image image = LoadImageFromTexture(texture);
	if (image.data == nullptr) return { nullptr, 0u };
	int size = 0;
	unsigned char* bytes = ExportImageToMemory(image, ".png", &size);
	UnloadImage(image);
	if (bytes == nullptr || size <= 0) return { nullptr, 0u };
	return { bytes, static_cast<std::size_t>(size) };
}

template <>
bool basic_TextureEmptySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture& texture,
		unsigned int width, unsigned int height,
		cgss::backend::RaylibBackend::native_image& image) {
	unloadTextureAndImage(texture, image);
	image = GenImageColor(static_cast<int>(width), static_cast<int>(height), ::Color{ 0, 0, 0, 0 });
	if (image.data == nullptr) return false;
	texture = LoadTextureFromImage(image);
	return texture.id != 0;
}

#ifdef LITECGSS_USE_PHYSFS
template <>
bool basic_TextureAssetSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture& texture,
		const std::string& filename,
		cgss::backend::RaylibBackend::native_image& image) {
	unloadTextureAndImage(texture, image);
	if (!AssetFile::exists(filename)) {
		image = LoadImage(filename.c_str());
	} else {
		try {
			const auto buffer = AssetFile{ filename, "r" }.fullLoad();
			image = LoadImageFromMemory(".png", buffer.data(), static_cast<int>(buffer.size()));
		} catch (const std::exception&) {
			return false;
		}
	}
	if (image.data == nullptr) return false;
	texture = LoadTextureFromImage(image);
	return texture.id != 0;
}

template <>
unsigned int basic_TextureAssetSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture& texture,
		const std::string& filename) {
	if (texture.id == 0) return 1u;
	::Image image = LoadImageFromTexture(texture);
	if (image.data == nullptr) return 1u;
	int size = 0;
	unsigned char* bytes = ExportImageToMemory(image, ".png", &size);
	UnloadImage(image);
	if (bytes == nullptr || size <= 0) return 1u;
	unsigned int result = 1u;
	try {
		AssetFile file{ filename, "w" };
		file.writeRaw(bytes, size);
		result = 0u;
	} catch (const std::exception&) {
		result = 1u;
	}
	MemFree(bytes);
	return result;
}
#else
template <>
bool basic_TextureFilesystemSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture& texture,
		const std::string& filename,
		cgss::backend::RaylibBackend::native_image& image) {
	unloadTextureAndImage(texture, image);
	image = LoadImage(filename.c_str());
	if (image.data == nullptr) return false;
	texture = LoadTextureFromImage(image);
	return texture.id != 0;
}

template <>
unsigned int basic_TextureFilesystemSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture& texture,
		const std::string& filename) {
	if (texture.id == 0) return 1u;
	::Image image = LoadImageFromTexture(texture);
	if (image.data == nullptr) return 1u;
	const bool ok = ExportImage(image, filename.c_str());
	UnloadImage(image);
	return ok ? 0u : 1u;
}
#endif

}
