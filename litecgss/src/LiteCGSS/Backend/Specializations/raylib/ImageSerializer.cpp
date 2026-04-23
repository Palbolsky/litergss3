// Raylib implementations of the Image serializer family. Replaces the
// earlier stubs with real LoadImage / LoadImageFromMemory / GenImageColor
// paths + raylib's ExportImageToMemory for the PNG save path. Selected by
// CMake when CGSS_BACKEND=raylib.

#include <cstdlib>
#include <cstring>

#include "LiteCGSS/Backend/RaylibBackend.h"
#include "LiteCGSS/Common/Serializer.h"
#include "LiteCGSS/Image/Serializers/ImageSerializer.h"

namespace cgss {

	// Memory → Image: raylib's LoadImageFromMemory detects the file-type
	// header automatically (PNG/JPG/etc.) so we pass the generic ".png"
	// extension hint; raylib ignores it when autodetect succeeds.
	template <>
	bool basic_ImageMemorySerializeMethod<cgss::backend::RaylibBackend>::load(
			cgss::backend::RaylibBackend::native_image& image,
			const MemorySerializerData& rawData) {
		if (image.data != nullptr) UnloadImage(image);
		image = LoadImageFromMemory(".png",
		                            reinterpret_cast<const unsigned char*>(rawData.first),
		                            static_cast<int>(rawData.second));
		return image.data != nullptr;
	}

	// Image → memory (PNG-encoded). raylib's ExportImageToMemory allocates
	// with its internal RL_MALLOC — the caller (Saver) owns the buffer and
	// must free via `MemFree`. MemorySerializerData's standard lifecycle
	// expects `std::free`, which we use here: raylib's default RL_MALLOC
	// is plain malloc, so std::free is compatible. If raylib is ever
	// configured with a custom allocator this deallocation path breaks —
	// document as a config-coupled assumption.
	template <>
	MemorySerializerData basic_ImageMemorySerializeMethod<cgss::backend::RaylibBackend>::save(
			const cgss::backend::RaylibBackend::native_image& image) {
		if (image.data == nullptr) return { nullptr, 0u };
		int size = 0;
		unsigned char* bytes = ExportImageToMemory(image, ".png", &size);
		if (bytes == nullptr || size <= 0) return { nullptr, 0u };
		return { bytes, static_cast<std::size_t>(size) };
	}

	// Empty RGBA image of the given dimensions, all pixels transparent.
	template <>
	bool basic_ImageEmptySerializeMethod<cgss::backend::RaylibBackend>::load(
			cgss::backend::RaylibBackend::native_image& image,
			unsigned int width, unsigned int height) {
		if (image.data != nullptr) UnloadImage(image);
		// `::Color{0,0,0,0}` in place of BLANK — the BLANK macro expands
		// to `Color{...}` which resolves to cgss::Color via namespace
		// lookup inside this TU.
		image = GenImageColor(static_cast<int>(width), static_cast<int>(height), ::Color{ 0, 0, 0, 0 });
		return image.data != nullptr;
	}

#ifdef LITECGSS_USE_PHYSFS
	// PhysFS path: the Asset layer reads the file into a buffer; we route
	// the buffer through LoadImageFromMemory. A direct-filesystem fallback
	// uses raylib's LoadImage when PhysFS can't locate the asset.
	template <>
	bool basic_ImageAssetSerializeMethod<cgss::backend::RaylibBackend>::load(
			cgss::backend::RaylibBackend::native_image& image,
			const std::string& filename) {
		if (!AssetFile::exists(filename)) {
			if (image.data != nullptr) UnloadImage(image);
			image = LoadImage(filename.c_str());
			return image.data != nullptr;
		}
		try {
			const auto buffer = AssetFile{ filename, "r" }.fullLoad();
			if (image.data != nullptr) UnloadImage(image);
			image = LoadImageFromMemory(".png", buffer.data(), static_cast<int>(buffer.size()));
			return image.data != nullptr;
		} catch (const std::exception&) {
			return false;
		}
	}

	// Encode to PNG, write via PhysFS (AssetFile). Returns 0 on success to
	// match the existing SFML convention (non-zero = error).
	template <>
	unsigned int basic_ImageAssetSerializeMethod<cgss::backend::RaylibBackend>::save(
			const cgss::backend::RaylibBackend::native_image& image,
			const std::string& filename) {
		if (image.data == nullptr) return 1u;
		int size = 0;
		unsigned char* bytes = ExportImageToMemory(image, ".png", &size);
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
	// Plain filesystem: LoadImage / ExportImage handle file I/O directly.
	template <>
	bool basic_ImageFilesystemSerializeMethod<cgss::backend::RaylibBackend>::load(
			cgss::backend::RaylibBackend::native_image& image,
			const std::string& filename) {
		if (image.data != nullptr) UnloadImage(image);
		image = LoadImage(filename.c_str());
		return image.data != nullptr;
	}

	template <>
	unsigned int basic_ImageFilesystemSerializeMethod<cgss::backend::RaylibBackend>::save(
			const cgss::backend::RaylibBackend::native_image& image,
			const std::string& filename) {
		if (image.data == nullptr) return 1u;
		return ExportImage(image, filename.c_str()) ? 0u : 1u;
	}
#endif

}
