// Backend-agnostic body for the Image serializer family. All PNG codec /
// file-I/O / blank-image creation routes through Ops::png_* and
// Ops::image_create_blank_rgba — each backend provides the matching thunks
// in its *Backend.h. The PhysFS path stays here (asset layer is shared).

#include <iostream>
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Image/Serializers/ImageSerializer.h"

namespace cgss {

	using ActiveBackend = cgss::backend::ActiveBackend;
	using Ops = ActiveBackend::Ops;

	template <>
	bool basic_ImageMemorySerializeMethod<ActiveBackend>::load(ActiveBackend::native_image& image, const MemorySerializerData& rawData) {
		return Ops::png_decode_memory(image, rawData.first, rawData.second);
	}

	template <>
	MemorySerializerData basic_ImageMemorySerializeMethod<ActiveBackend>::save(const ActiveBackend::native_image& image) {
		return Ops::png_encode_memory(image);
	}

	template <>
	bool basic_ImageEmptySerializeMethod<ActiveBackend>::load(ActiveBackend::native_image& image, unsigned int width, unsigned int height) {
		return Ops::image_create_blank_rgba(image, width, height);
	}

#ifdef LITECGSS_USE_PHYSFS
	template <>
	bool basic_ImageAssetSerializeMethod<ActiveBackend>::load(ActiveBackend::native_image& image, const std::string& filename) {
		if (!AssetFile::exists(filename)) {
			return Ops::png_decode_file(image, filename.c_str());
		}
		try {
			const auto buffer = AssetFile { filename, "r" }.fullLoad();
			return Ops::png_decode_memory(image, buffer.data(), buffer.size());
		} catch (const std::exception& err) {
			std::cerr << "Error while loading asset \"" << filename << "\" : " << err.what() << std::endl;
			return false;
		}
	}

	template <>
	unsigned int basic_ImageAssetSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_image& image, const std::string& filename) {
		// Currently unsupported in an archive — fall through to direct file
		// write. Should we route via AssetFile{...,"w"} instead?
		return Ops::png_encode_file(image, filename.c_str());
	}
#else
	template <>
	bool basic_ImageFilesystemSerializeMethod<ActiveBackend>::load(ActiveBackend::native_image& image, const std::string& filename) {
		return Ops::png_decode_file(image, filename.c_str());
	}

	template <>
	unsigned int basic_ImageFilesystemSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_image& image, const std::string& filename) {
		return Ops::png_encode_file(image, filename.c_str());
	}
#endif

}
