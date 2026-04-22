// Raylib stubs for the Texture serializer family. All return false / empty
// MemorySerializerData. Real raylib impls would route through LoadImage /
// LoadImageFromMemory / GenImageColor + LoadTextureFromImage / UpdateTexture
// (the lodepng decode/encode bookkeeping in the SFML body is already
// backend-agnostic and reusable when those land).

#include "LiteCGSS/Backend/RaylibBackend.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"

namespace cgss {

template <>
bool basic_TextureMemorySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture&,
		const MemorySerializerData&,
		cgss::backend::RaylibBackend::native_image&) {
	return false;
}

template <>
MemorySerializerData basic_TextureMemorySerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture&) {
	return { nullptr, 0u };
}

template <>
bool basic_TextureEmptySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture&,
		unsigned int, unsigned int,
		cgss::backend::RaylibBackend::native_image&) {
	return false;
}

#ifdef LITECGSS_USE_PHYSFS
template <>
bool basic_TextureAssetSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture&,
		const std::string&,
		cgss::backend::RaylibBackend::native_image&) {
	return false;
}

template <>
unsigned int basic_TextureAssetSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture&,
		const std::string&) {
	return 1u;
}
#else
template <>
bool basic_TextureFilesystemSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_texture&,
		const std::string&,
		cgss::backend::RaylibBackend::native_image&) {
	return false;
}

template <>
unsigned int basic_TextureFilesystemSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_texture&,
		const std::string&) {
	return 1u;
}
#endif

}
