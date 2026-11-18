// Backend-agnostic body for the Texture serializer family. The CPU image
// is decoded via Ops::png_decode_*, then uploaded to the GPU texture via
// Ops::texture_create_from_image. Saving reads the GPU texels back via
// Ops::texture_read_to_image and then encodes them with Ops::png_encode_*.
// Each backend's *Backend.h thunks hide the native upload / read-back calls
// and the matching memory lifecycle.

#include <iostream>
#include "Common/Profiling.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Common/VFS.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"
#include "Graphics/Serializers/TextureUploadDiagnostic.h"

namespace cgss {

	using ActiveBackend = cgss::backend::ActiveBackend;
	using Ops = ActiveBackend::Ops;

	namespace {
		// A load's three steps, each its own profiler zone: reading the file,
		// decoding the PNG (CPU), uploading the pixels (GPU)
		bool uploadImage(ActiveBackend::native_texture& texture, const ActiveBackend::native_image& image) {
			CGSS_PROFILE_ZONE("cgss::texture upload");
			return detail::uploadImage<Ops>(texture, image);
		}
		bool decodePngFile(ActiveBackend::native_image& image, const std::string& filename) {
			CGSS_PROFILE_ZONE("cgss::png decode (file)");
			return Ops::png_decode_file(image, filename.c_str());
		}
		bool decodePngMemory(ActiveBackend::native_image& image, const void* data, std::size_t size) {
			CGSS_PROFILE_ZONE("cgss::png decode");
			return Ops::png_decode_memory(image, data, size);
		}
	}

	template <>
	bool basic_TextureMemorySerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const MemorySerializerData& rawData, ActiveBackend::native_image& image) {
	CGSS_PROFILE_ZONE("cgss::TextureMemorySerializeMethod::load");
		Ops::texture_destroy(texture);
		if (!decodePngMemory(image, rawData.first, rawData.second)) return false;
		return uploadImage(texture, image);
	}

	template <>
	MemorySerializerData basic_TextureMemorySerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture) {
	CGSS_PROFILE_ZONE("cgss::TextureMemorySerializeMethod::save");
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return { nullptr, 0u };
		auto out = Ops::png_encode_memory(image);
		Ops::image_destroy(image);
		return out;
	}

	template <>
	bool basic_TextureEmptySerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, unsigned int width, unsigned int height, ActiveBackend::native_image& image) {
	CGSS_PROFILE_ZONE("cgss::TextureEmptySerializeMethod::load");
		Ops::texture_destroy(texture);
		if (!Ops::image_create_blank_rgba(image, width, height)) return false;
		return uploadImage(texture, image);
	}

#ifdef LITECGSS_USE_PHYSFS
	template <>
	bool basic_TextureAssetSerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const std::string& filename, ActiveBackend::native_image& image) {
	CGSS_PROFILE_ZONE("cgss::TextureAssetSerializeMethod::load");
		Ops::texture_destroy(texture);
		bool ok = false;
		if (!cgss::vfs::exists(filename)) {
			ok = decodePngFile(image, filename);
		} else {
			try {
				std::vector<char> buffer;
				{
					CGSS_PROFILE_ZONE("cgss::vfs read");
					buffer = cgss::vfs::loadFully(filename);
				}
				ok = decodePngMemory(image, buffer.data(), buffer.size());
			} catch (const std::exception& err) {
				std::cerr << "Error while loading asset \"" << filename << "\" : " << err.what() << std::endl;
				return false;
			}
		}
		if (!ok) return false;
		return uploadImage(texture, image);
	}

	template <>
	unsigned int basic_TextureAssetSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture, const std::string& filename) {
	CGSS_PROFILE_ZONE("cgss::TextureAssetSerializeMethod::save");
		// Saves go to the real filesystem, not into the mounted archives
		// (as LiteCGSS2 does): a game writes its files through its own IO.
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return 1u;
		const unsigned int rc = Ops::png_encode_file(image, filename.c_str());
		Ops::image_destroy(image);
		return rc;
	}
#else
	template <>
	bool basic_TextureFilesystemSerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const std::string& filename, ActiveBackend::native_image& image) {
	CGSS_PROFILE_ZONE("cgss::TextureFilesystemSerializeMethod::load");
		Ops::texture_destroy(texture);
		if (!decodePngFile(image, filename)) return false;
		return uploadImage(texture, image);
	}

	template <>
	unsigned int basic_TextureFilesystemSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture, const std::string& filename) {
	CGSS_PROFILE_ZONE("cgss::TextureFilesystemSerializeMethod::save");
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return 1u;
		const unsigned int rc = Ops::png_encode_file(image, filename.c_str());
		Ops::image_destroy(image);
		return rc;
	}
#endif

}
