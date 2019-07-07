#ifndef LR_SHAPE_H
#define LR_SHAPE_H
#include "Graphics/Drawable.h"
#include "Common/Meta/metadata.h"

namespace lr {
	class Shape : 
		public Drawable<Shape> {
	public:
		Shape() = default;
		virtual ~Shape() = default;

		void draw(sf::RenderTarget& target) const override;
		void drawFast(sf::RenderTarget& target) const override;

		sf::Shape* getShape() { return m_shape.get(); }
		template <class ShapeC, class ... Args>
		void setShape(Args&& ... args) {
			m_shape = std::make_unique<ShapeC>(std::forward<Args>(args)...);
		}

		void setRenderState(sf::RenderStates* states);

	private:
		std::unique_ptr<sf::Shape> m_shape;
		sf::RenderStates* m_renderStates = nullptr;
	};

	namespace meta {
		template<>
		struct Log<Shape> {
			static constexpr auto classname = "Shape";
		};
	}
}
#endif