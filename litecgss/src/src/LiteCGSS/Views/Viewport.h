#ifndef CGSS_VIEWPORT_H
#define CGSS_VIEWPORT_H
#include <optional>
#include "LiteCGSS/Graphics/DrawState.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/ShaderRenderable.h"
#include "SnapshotCapturable.h"
#include "View.h"
#include "ViewAuthorizations.h"
#include "ViewBox.h"
#include "DisplayWindowUserData.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/SpriteFwd.h"

namespace cgss {

	class Viewport;
	class ViewportItem :
		public GraphicsStackItem,
		public View,
		public ViewBox,
		public ShaderRenderable {
		friend class Viewport;
	private:
		void setupFullWindowView(cgss::RenderTarget& target) const;
		void scaleRenderSpriteToViewport() const;
		const RenderStatesData* getRenderStates() const;
		RenderStatesData* getRenderStates();

		DisplayWindowUserData m_windowData;

		std::optional<cgss::DrawState> m_renderStates;
		std::unique_ptr<RenderStates> m_defaultRenderStates;

		double m_wantedOx = 0;
		double m_wantedOy = 0;
	public:
		ViewportItem(Viewport* owner, MainEventDispatcher& eventDispatcher, DisplayWindowUserData windowData, const DisplayWindowSettings& windowSettings);
		void drawFast(cgss::RenderTarget& target) const override { draw(target); }
		void draw(cgss::RenderTarget& target) const override;

		void updateFromValue(const cgss::IntRect* rectangle) override;
		void updateFromValue(const RenderStatesData* renderStatesData) override;
		cgss::IntRect getViewportBox() const;
		std::unique_ptr<cgss::Texture> takeSnapshot() const;

	};

	struct DisplayWindowSettings;
	class Viewport :
		public Drawable<Viewport, ViewportItem>,
		public GraphicItem,
		public SnapshotCapturable {

	public:
		Viewport() = default;
		Viewport(Viewport&&) noexcept = default;
		Viewport& operator=(Viewport&&) noexcept = default;
		Viewport(const Viewport&) = delete;
		Viewport& operator=(const Viewport&) = delete;
		virtual ~Viewport();

		void bindRenderStates(RenderStates* states);
		std::unique_ptr<cgss::Texture> takeSnapshot() const override;
		void drawFast(cgss::RenderTarget& target) const override;
		void draw(cgss::RenderTarget& target) const override;
		void sortZ();

		void moveOrigin(long ox, long oy);
		void move(float x, float y);
		void resize(long width, long height);
		void setRectangle(cgss::IntRect rectangle);
		cgss::IntRect getViewportBox() const;

		long getOx() const;
		long getOy() const;

		void setAngle(float angle);
		void setZoom(float zoom);

		template <class View, class StackItem, class Drawable>
		StackItem& add(std::shared_ptr<StackItem> d) {
			return m_proxy->View::add<Viewport, StackItem, Drawable>(std::move(d));
		}

		template <class View, class ... Args>
		View addView(Args&& ... args) {
			return View::create(*this, m_proxy->m_windowData.eventDispatcher, m_proxy->m_windowData, m_proxy->m_windowData.settings, std::forward<Args>(args)...);
		}

	private:
		void onDispose() override;
	};

	class Text;
	class Shape;
	class FramedView;
	class SpriteMap;

	template <>
	struct ViewAuthorizations<Viewport> : public AuthorizedDrawableSet<Sprite, SpriteMap, Text, Shape, FramedView> {};

	namespace meta {
		template<>
		struct Log<Viewport> {
			static constexpr auto classname = "Viewport";
		};
	}
}

#endif
