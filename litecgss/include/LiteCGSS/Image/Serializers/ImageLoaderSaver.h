#ifndef CGSS_IMAGE_LOADER_SAVER_H
#define CGSS_IMAGE_LOADER_SAVER_H

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Loader.h"
#include "LiteCGSS/Common/Saver.h"

namespace cgss {

	template <class B>
	using basic_ImageLoader = Loader<typename B::native_image>;

	template <class B>
	using basic_ImageSaver  = Saver<typename B::native_image>;

	using ImageLoader = basic_ImageLoader<cgss::backend::ActiveBackend>;
	using ImageSaver  = basic_ImageSaver<cgss::backend::ActiveBackend>;
}

#endif
