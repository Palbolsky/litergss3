#ifndef CGSS_FRAMED_VIEW_H
#define CGSS_FRAMED_VIEW_H

#include "View.h"
#include "ViewAuthorizations.h"
#include "Graphics/Texture.h"
#include "Graphics/Drawable.h"

#include "Events/MainEventDispatcher.h"
#include "Events/Patterns/SubObserver.h"
#include "ViewBox.h"

namespace cgss {
	
	class FramedView;
	class FramedViewItem : 
		public GraphicsStackItem,
		public View,
		public ViewBox {
		friend class FramedView;

	private:
		void onMove() override final;
		void onResize() override final;

		void updateCursorSprite();

		void updatePauseSprite();
		void resetPausePosition();

		void updateContents(std::unique_ptr<sf::IntRect> cursorRect = nullptr);
		void updateBackOpacity();
		void updateContentsOpacity();

		std::vector<sf::VertexArray> m_vertices;
		sf::Texture* m_texture = nullptr;
		sf::View m_view;
		sf::Sprite m_pause;
		Texture m_pauseSkin;
		sf::Sprite m_cursor;

		unsigned long m_numVerticesLine = 0;
		long m_counter = 0;

		bool m_locked = false;
		bool m_paused = false;
		bool m_stretch = false;

		//Added since LiteRGSS split (those were Ruby variables before)
		long m_x = 0;
		long m_y = 0;
		long m_width = -1L;
		long m_height = -1L;
		float m_zoom = 1.F;

		std::unique_ptr<sf::IntRect> m_cursorRect;
		long m_windowBuilder[6] = {};
		std::uint8_t m_opacity = 255;
		std::uint8_t m_backOpacity = 255;
		std::uint8_t m_contentOpacity = 255;
		bool m_active = false;
		sf::IntRect m_rect;
		std::optional<long> m_pauseX;
		std::optional<long> m_pauseY;

		void updateVerticesBlt(long wt, long ht);
		void updateVerticesStretch(long wt, long ht);
		void allocateVerticesBlt(long delta_w, long nb2, long delta_h, long nb4);
		void calculateVertices(long x, long y, long line, long cell, sf::Vector2i& a, sf::IntRect& rect);
		void rectSet(sf::IntRect& rect, long x, long y, long width, long height);
		void allocateVerticesStretch();
		void calculateVerticesStretch(long x, long y, long line, long cell, sf::Vector2i& s, sf::Vector2i& a, sf::IntRect& rect);

	public:
		FramedViewItem(FramedView* owner, MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings);
		FramedViewItem(FramedViewItem&&) noexcept = default;
		FramedViewItem& operator=(FramedViewItem&&) noexcept = default;
		FramedViewItem(const FramedViewItem&) = delete;
		FramedViewItem& operator=(const FramedViewItem&) = delete;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;
		void drawCalculateView(sf::RenderTarget& target, sf::View& targetView) const;
		void update();

		void setOpacity(uint8_t opacity);
		uint8_t getOpacity() const;

		void setContentsOpacity(uint8_t opacity);
		uint8_t getContentsOpacity() const;

		void lock();
		void unlock();
		bool isLocked() const;

		void pause(bool pause);
		bool isPaused() const;

		void active(bool active);
		bool isActive() const;

		void setSkin(sf::Texture* skin);
		sf::Texture* getSkin() const;

		void setPauseSkin(sf::Texture& texture);
		void setPausePosition(float x, float y);

		void setCursorSkin(sf::Texture& texture);
		void setCursorPosition(sf::IntRect position);

		void stretch(bool stretch);
		bool isStretched() const;

	protected:
		void updateVertices();
		void updateView();
	};

	class FramedView :
		public Drawable<FramedView, FramedViewItem>,
		public GraphicItem {
	public:
		FramedView() = default;
		virtual ~FramedView() = default;

		FramedView(FramedView&&) noexcept = default;
		FramedView& operator=(FramedView&&) noexcept = default;
		FramedView(const FramedView&) = delete;
		FramedView& operator=(const FramedView&) = delete;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		void lock();
		void unlock();
		bool isLocked() const;
	};

	class Sprite;
	class Text;
	class Shape;

	template <>
	struct ViewAuthorizations<FramedView> : public AuthorizedDrawableSet<Sprite, Text, Shape> {};
}

#endif