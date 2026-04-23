#ifndef CGSS_TEXTURE_H
#define CGSS_TEXTURE_H

#include <array>
#include <memory>
#include <string>
#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "LiteCGSS/Image/ImageFwd.h"
#include "LiteCGSS/Graphics/Serializers/TextureLoaderSaver.h"

#include "LiteCGSS/Views/DisplayWindowFwd.h"

namespace cgss {
	class Rectangle;

	using TextureFactorNpot = std::array<double, 2>;

	// Templated wrapper over `Backend::native_texture` (+ a backing image
	// for software pixel ops). Consumers refer to it as `cgss::Texture` —
	// the alias near the bottom of this header binds B to the build-selected
	// backend. All special members and methods are declared (not defined)
	// here; the bodies live in Texture.cpp where the full backend header
	// (with native types complete) is available.
	template <class B>
	class basic_Texture {
		friend struct cgss::backend::ActiveBackend::Ops;
	public:
		using native_type = typename B::native_texture;

		basic_Texture();
		basic_Texture(const basic_Texture& other);
		basic_Texture(basic_Texture&& other) noexcept;
		basic_Texture& operator=(const basic_Texture& other);
		basic_Texture& operator=(basic_Texture&& other) noexcept;
		virtual ~basic_Texture();

		static basic_Texture create(const DisplayWindow& window, const std::string& filename);
		static basic_Texture create(const std::string& filename);

		// Backend-coupling hatch: adopt an existing native texture. Only meant
		// for internal callers that already operate in backend-specific terms
		// (e.g. snapshot capture). Portable code should not use this.
		static basic_Texture create_from_native(typename B::native_texture native);

		void update();
		void update(const basic_Image<B>& image);

		struct Size { unsigned int x; unsigned int y; };
		Size getSize() const;

		void blit(unsigned long x, unsigned long y, const basic_Texture& srcTexture, const Rectangle& rect);
		void fillRect(long x, long y, unsigned int width, unsigned int height, cgss::Color color);
		void clearRect(long x, long y, unsigned int width, unsigned int height);
		void setSmooth(bool smooth);

		basic_Texture clone() const;

		bool load(const TextureLoader& loader);
		unsigned int write(TextureSaver& saver) const;

		bool isLoaded() const { return m_loaded; }

		bool create(int sw, int sh);

		// NOTE: the public `ref()` / `weak()` escape hatches that used to
		// live here have been removed (step B of the abstraction plan). The
		// shared_ptr<native_texture> they returned was a clean-boundary
		// violation. Internal cgss users (Sprite / Shape setTexture) now go
		// through `Ops::texture_share(tex)`. External consumers that
		// legitimately need a native handle (video decoders, platform
		// plugins) should include `LiteCGSS/Extension/NativeAccess.h` and
		// use `cgss::extension::native(tex)` — opt-in via CGSS_ENABLE_EXTENSION_SPI.

		TextureFactorNpot factorNpot() const;

	private:
		native_type& native();
		const native_type& native() const;

		static unsigned int getNpotSize(unsigned int size);
		bool hasImage() const;

		bool m_loaded = false;
		std::shared_ptr<typename B::native_texture> m_texture;
		std::unique_ptr<typename B::native_image> m_image;
	};

	using Texture = basic_Texture<cgss::backend::ActiveBackend>;

	namespace meta {
		template <class B>
		struct Log<basic_Texture<B>> {
			static constexpr auto classname = "Texture";
		};
	}
}

#endif
