#ifndef CGSS_DISPLAY_WINDOW_DRAW_H
#define CGSS_DISPLAY_WINDOW_DRAW_H
#include <memory>

namespace sf {
	class RenderTexture;
}

namespace cgss {
	class DrawState;

	struct DisplayWindowDrawData {
		const cgss::DrawState* renderState = nullptr;
		std::unique_ptr<sf::RenderTexture> renderTexture;
		unsigned long frameRate = 60;
		unsigned char brightness = 255;
		long screenWidth = 640;
		long screenHeight = 480;
		double scale = 1;
		bool smoothScreen = false;

		// All special member functions declared out-of-line so that this struct's
		// destructor is not implicitly instantiated in every TU that includes this
		// header — sf::RenderTexture is only fwd-declared here.
		DisplayWindowDrawData();
		~DisplayWindowDrawData();
		DisplayWindowDrawData(DisplayWindowDrawData&&) noexcept;
		DisplayWindowDrawData& operator=(DisplayWindowDrawData&&) noexcept;
		DisplayWindowDrawData(const DisplayWindowDrawData&) = delete;
		DisplayWindowDrawData& operator=(const DisplayWindowDrawData&) = delete;
	};
}
#endif
