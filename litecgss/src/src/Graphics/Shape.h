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
	
	private:
		std::unique_ptr<sf::Shape> m_shape;
	};

	namespace meta {
		template<>
		struct Log<Shape> {
			static constexpr auto classname = "Shape";
		};
	}
}
#endif