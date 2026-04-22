#ifndef CGSS_IMAGE_SERIALIZER_H
#define CGSS_IMAGE_SERIALIZER_H

#include "LiteCGSS/Image/Serializers/ImageLoaderSaver.h"
#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Serializer.h"

namespace cgss {
#ifdef LITECGSS_USE_PHYSFS
	template <class B>
	struct basic_ImageAssetSerializeMethod {
		static bool load(typename B::native_image& image, const std::string& filename);
		static unsigned int save(const typename B::native_image& image, const std::string& filename);
	};
	template <class B>
	using basic_ImageFileSerializeMethod = basic_ImageAssetSerializeMethod<B>;
#else
	template <class B>
	struct basic_ImageFilesystemSerializeMethod {
		static bool load(typename B::native_image& image, const std::string& filename);
		static unsigned int save(const typename B::native_image& image, const std::string& filename);
	};
	template <class B>
	using basic_ImageFileSerializeMethod = basic_ImageFilesystemSerializeMethod<B>;
#endif

	template <class B>
	using basic_ImageFileSerializer = FileSerializer<basic_ImageFileSerializeMethod<B>, typename B::native_image, basic_ImageLoader<B>, basic_ImageSaver<B>>;

	template <class B>
	struct basic_ImageMemorySerializeMethod {
		static bool load(typename B::native_image& image, const MemorySerializerData& rawData);
		static MemorySerializerData save(const typename B::native_image& image);
	};

	template <class B>
	using basic_ImageMemorySerializer = MemorySerializer<basic_ImageMemorySerializeMethod<B>, typename B::native_image, basic_ImageLoader<B>, basic_ImageSaver<B>>;

	template <class B>
	struct basic_ImageEmptySerializeMethod {
		static bool load(typename B::native_image& image, unsigned int width, unsigned int height);
	};

	template <class B>
	using basic_ImageEmptySerializer = Empty2DSerializer<basic_ImageEmptySerializeMethod<B>, typename B::native_image, basic_ImageLoader<B>>;

#ifdef LITECGSS_USE_PHYSFS
	using ImageAssetSerializeMethod = basic_ImageAssetSerializeMethod<cgss::backend::ActiveBackend>;
#else
	using ImageFilesystemSerializeMethod = basic_ImageFilesystemSerializeMethod<cgss::backend::ActiveBackend>;
#endif
	using ImageFileSerializeMethod    = basic_ImageFileSerializeMethod<cgss::backend::ActiveBackend>;
	using ImageFileSerializer         = basic_ImageFileSerializer<cgss::backend::ActiveBackend>;
	using ImageMemorySerializeMethod  = basic_ImageMemorySerializeMethod<cgss::backend::ActiveBackend>;
	using ImageMemorySerializer       = basic_ImageMemorySerializer<cgss::backend::ActiveBackend>;
	using ImageEmptySerializeMethod   = basic_ImageEmptySerializeMethod<cgss::backend::ActiveBackend>;
	using ImageEmptySerializer        = basic_ImageEmptySerializer<cgss::backend::ActiveBackend>;
}
#endif
