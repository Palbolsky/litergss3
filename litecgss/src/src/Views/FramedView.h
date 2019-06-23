#ifndef LR_FRAMED_VIEW_H
#define LR_FRAMED_VIEW_H

#include "View.h"
#include "ViewAuthorizations.h"
#include "Graphics/Texture.h"

namespace lr {
	class FramedView :
		public View {
	private:
		std::vector<sf::VertexArray> m_vertices;
		sf::Texture* m_texture = nullptr;
		sf::View m_view;
		bool m_visible = true;
		unsigned long m_numVerticesLine = 0;
		long m_counter = 0;
		sf::Sprite m_pause;
		Texture m_pauseSkin;
		sf::Sprite m_cursor;
		bool m_locked = false;

		//Added since LiteRGSS split (those were Ruby variables before)
		float m_zoom = 1.F;
		long m_width = -1L;
		long m_height = -1L;
		bool m_stretch = false;
		std::unique_ptr<sf::IntRect> m_cursorRect;
		long m_x = 0;
		long m_y = 0;
		long m_windowBuilder[6] = {};
		std::uint8_t m_opacity = 255;
		std::uint8_t m_backOpacity = 255;
		std::uint8_t m_contentOpacity = 255;
		bool m_active = false;
		sf::IntRect m_rect;
		long m_screenWidth = 600;
		long m_screenHeight = 240;
		long m_OX = 0;
		long m_OY = 0;
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
		FramedView() = default;
		virtual ~FramedView() = default;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;
		void drawCalculateView(sf::RenderTarget& target, sf::View& targetView) const;

		void update();
		void updatePauseSprite();
		void updateCursorSprite();
		void updateVertices();
		void updateContents();
		void updateBackOpacity();
		void updateContentsOpacity();
		void updateView();
		void lock();
		void unlock();
		bool is_locked();
		sf::Sprite& getPauseSprite();
		sf::Sprite& getCursorSprite();
		void resetPausePosition();
		void resetCursorPosition(sf::IntRect* rect);
	};

	class Sprite;
	class Text;
	class Shape;

	template <>
	struct ViewAuthorizations<FramedView> : public AuthorizedDrawableSet<Sprite, Text, Shape> {};
}

#endif