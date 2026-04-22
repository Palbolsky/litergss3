// Raylib stubs for the Image serializer family. All return false / empty
// MemorySerializerData. Real raylib impls would route through LoadImage /
// LoadImageFromMemory / GenImageColor; encode-to-PNG via lodepng is already
// backend-agnostic and reusable when those land.

#include "LiteCGSS/Backend/RaylibBackend.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Image/Serializers/ImageSerializer.h"

namespace cgss {

template <>
bool basic_ImageMemorySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_image&,
		const MemorySerializerData&) {
	return false;
}

template <>
MemorySerializerData basic_ImageMemorySerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_image&) {
	return { nullptr, 0u };
}

template <>
bool basic_ImageEmptySerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_image&,
		unsigned int, unsigned int) {
	return false;
}

#ifdef LITECGSS_USE_PHYSFS
template <>
bool basic_ImageAssetSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_image&,
		const std::string&) {
	return false;
}

template <>
unsigned int basic_ImageAssetSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_image&,
		const std::string&) {
	return 1u;
}
#else
template <>
bool basic_ImageFilesystemSerializeMethod<cgss::backend::RaylibBackend>::load(
		cgss::backend::RaylibBackend::native_image&,
		const std::string&) {
	return false;
}

template <>
unsigned int basic_ImageFilesystemSerializeMethod<cgss::backend::RaylibBackend>::save(
		const cgss::backend::RaylibBackend::native_image&,
		const std::string&) {
	return 1u;
}
#endif

}
