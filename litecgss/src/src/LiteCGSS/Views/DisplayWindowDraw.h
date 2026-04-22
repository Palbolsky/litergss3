#ifndef CGSS_DISPLAY_WINDOW_DRAW_H
#define CGSS_DISPLAY_WINDOW_DRAW_H
#include <memory>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Graphics/DrawStateFwd.h"

namespace cgss {

	template <class B>
	struct basic_DisplayWindowDrawData {
		const cgss::DrawState* renderState = nullptr;
		std::unique_ptr<typename B::native_render_texture> renderTexture;
		unsigned long frameRate = 60;
		unsigned char brightness = 255;
		long screenWidth = 640;
		long screenHeight = 480;
		double scale = 1;
		bool smoothScreen = false;

		// All special member functions declared out-of-line so that this struct's
		// destructor is not implicitly instantiated in every TU that includes this
		// header — `typename B::native_render_texture` is only fwd-declared here
		// (the slim Fwd backend header doesn't pull the full native definitions).
		basic_DisplayWindowDrawData();
		~basic_DisplayWindowDrawData();
		basic_DisplayWindowDrawData(basic_DisplayWindowDrawData&&) noexcept;
		basic_DisplayWindowDrawData& operator=(basic_DisplayWindowDrawData&&) noexcept;
		basic_DisplayWindowDrawData(const basic_DisplayWindowDrawData&) = delete;
		basic_DisplayWindowDrawData& operator=(const basic_DisplayWindowDrawData&) = delete;
	};

	using DisplayWindowDrawData = basic_DisplayWindowDrawData<cgss::backend::ActiveBackend>;
}
#endif
