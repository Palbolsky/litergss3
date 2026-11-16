#ifndef CGSS_TEXTURE_SERIALIZER_H
#define CGSS_TEXTURE_SERIALIZER_H

#include "LiteCGSS/Graphics/Serializers/TextureLoaderSaver.h"
#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Serializer.h"

namespace cgss {
#ifdef LITECGSS_USE_PHYSFS
	template <class B>
	struct basic_TextureAssetSerializeMethod {
		static bool load(typename B::native_texture& texture, const std::string& filename, typename B::native_image& image);
		static unsigned int save(const typename B::native_texture& texture, const std::string& filename);
	};
	template <class B>
	using basic_TextureFileSerializeMethod = basic_TextureAssetSerializeMethod<B>;
#else
	template <class B>
	struct basic_TextureFilesystemSerializeMethod {
		static bool load(typename B::native_texture& texture, const std::string& filename, typename B::native_image& image);
		static unsigned int save(const typename B::native_texture& texture, const std::string& filename);
	};
	template <class B>
	using basic_TextureFileSerializeMethod = basic_TextureFilesystemSerializeMethod<B>;
#endif

	template <class B>
	using basic_TextureFileSerializer = FileSerializer<basic_TextureFileSerializeMethod<B>, typename B::native_texture, basic_TextureLoader<B>, basic_TextureSaver<B>>;

	template <class B>
	struct basic_TextureMemorySerializeMethod {
		static bool load(typename B::native_texture& texture, const MemorySerializerData& rawData, typename B::native_image& image);
		static MemorySerializerData save(const typename B::native_texture& texture);
	};

	template <class B>
	using basic_TextureMemorySerializer = MemorySerializer<basic_TextureMemorySerializeMethod<B>, typename B::native_texture, basic_TextureLoader<B>, basic_TextureSaver<B>>;

	template <class B>
	struct basic_TextureEmptySerializeMethod {
		static bool load(typename B::native_texture& texture, unsigned int width, unsigned int height, typename B::native_image& image);
	};

	template <class B>
	using basic_TextureEmptySerializer = Empty2DSerializer<basic_TextureEmptySerializeMethod<B>, typename B::native_texture, basic_TextureLoader<B>>;

#ifdef LITECGSS_USE_PHYSFS
	using TextureAssetSerializeMethod = basic_TextureAssetSerializeMethod<cgss::backend::ActiveBackend>;
#else
	using TextureFilesystemSerializeMethod = basic_TextureFilesystemSerializeMethod<cgss::backend::ActiveBackend>;
#endif
	using TextureFileSerializeMethod    = basic_TextureFileSerializeMethod<cgss::backend::ActiveBackend>;
	using TextureFileSerializer         = basic_TextureFileSerializer<cgss::backend::ActiveBackend>;
	using TextureMemorySerializeMethod  = basic_TextureMemorySerializeMethod<cgss::backend::ActiveBackend>;
	using TextureMemorySerializer       = basic_TextureMemorySerializer<cgss::backend::ActiveBackend>;
	using TextureEmptySerializeMethod   = basic_TextureEmptySerializeMethod<cgss::backend::ActiveBackend>;
	using TextureEmptySerializer        = basic_TextureEmptySerializer<cgss::backend::ActiveBackend>;
}
#endif
