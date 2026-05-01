// Backend-agnostic body for the Texture serializer family. The CPU image
// is decoded via Ops::png_decode_*, then uploaded to the GPU texture via
// Ops::texture_create_from_image. Saving reads the GPU texels back via
// Ops::texture_read_to_image and then encodes them with Ops::png_encode_*.
// Each backend's *Backend.h thunk hides the SFML (sf::Texture::loadFromImage,
// copyToImage) / raylib (LoadTextureFromImage, LoadImageFromTexture)
// difference + the matching memory lifecycle.

#include <iostream>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Assets.h"
#include "LiteCGSS/Graphics/Serializers/TextureSerializer.h"

namespace cgss {

	using ActiveBackend = cgss::backend::ActiveBackend;
	using Ops = ActiveBackend::Ops;

	template <>
	bool basic_TextureMemorySerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const MemorySerializerData& rawData, ActiveBackend::native_image& image) {
		Ops::texture_destroy(texture);
		if (!Ops::png_decode_memory(image, rawData.first, rawData.second)) return false;
		Ops::texture_create_from_image(texture, image);
		return true;
	}

	template <>
	MemorySerializerData basic_TextureMemorySerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture) {
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return { nullptr, 0u };
		auto out = Ops::png_encode_memory(image);
		Ops::image_destroy(image);
		return out;
	}

	template <>
	bool basic_TextureEmptySerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, unsigned int width, unsigned int height, ActiveBackend::native_image& image) {
		Ops::texture_destroy(texture);
		if (!Ops::image_create_blank_rgba(image, width, height)) return false;
		Ops::texture_create_from_image(texture, image);
		return true;
	}

#ifdef LITECGSS_USE_PHYSFS
	template <>
	bool basic_TextureAssetSerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const std::string& filename, ActiveBackend::native_image& image) {
		Ops::texture_destroy(texture);
		bool ok = false;
		if (!AssetFile::exists(filename)) {
			ok = Ops::png_decode_file(image, filename.c_str());
		} else {
			try {
				const auto buffer = AssetFile{ filename, "r" }.fullLoad();
				ok = Ops::png_decode_memory(image, buffer.data(), buffer.size());
			} catch (const std::exception& err) {
				std::cerr << "Error while loading asset \"" << filename << "\" : " << err.what() << std::endl;
				return false;
			}
		}
		if (!ok) return false;
		Ops::texture_create_from_image(texture, image);
		return true;
	}

	template <>
	unsigned int basic_TextureAssetSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture, const std::string& filename) {
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return 1u;
		const auto buffer = Ops::png_encode_memory(image);
		Ops::image_destroy(image);
		if (buffer.first == nullptr || buffer.second == 0u) return 1u;
		unsigned int result = 1u;
		try {
			AssetFile file{ filename, "w" };
			file.writeRaw(buffer.first, buffer.second);
			result = 0u;
		} catch (const std::exception&) {
			result = 1u;
		}
		std::free(buffer.first);
		return result;
	}
#else
	template <>
	bool basic_TextureFilesystemSerializeMethod<ActiveBackend>::load(ActiveBackend::native_texture& texture, const std::string& filename, ActiveBackend::native_image& image) {
		Ops::texture_destroy(texture);
		if (!Ops::png_decode_file(image, filename.c_str())) return false;
		Ops::texture_create_from_image(texture, image);
		return true;
	}

	template <>
	unsigned int basic_TextureFilesystemSerializeMethod<ActiveBackend>::save(const ActiveBackend::native_texture& texture, const std::string& filename) {
		ActiveBackend::native_image image{};
		if (!Ops::texture_read_to_image(texture, image)) return 1u;
		const unsigned int rc = Ops::png_encode_file(image, filename.c_str());
		Ops::image_destroy(image);
		return rc;
	}
#endif

}
