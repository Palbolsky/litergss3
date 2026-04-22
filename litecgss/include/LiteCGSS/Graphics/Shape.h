#ifndef CGSS_SHAPE_H
#define CGSS_SHAPE_H

#include <cstddef>
#include <memory>
#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Color.h"
#include "LiteCGSS/Common/IntRect.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Graphics/Drawable.h"
#include "LiteCGSS/Graphics/DrawState.h"
#include "LiteCGSS/Graphics/ShapeFwd.h"
#include "LiteCGSS/Graphics/ShapeGeometry.h"
#include "LiteCGSS/Graphics/TextureFwd.h"
#include "LiteCGSS/Graphics/Transformable.h"

namespace cgss {
	class View;

	template <class B>
	class basic_ShapeItem :
		public GraphicsStackItem {
		friend class basic_Shape<B>;
	private:
		ShapeGeometry m_geometry {};
		std::shared_ptr<typename B::native_texture> m_linkedTexture = nullptr;
		Transformable* m_transformable = nullptr;
		cgss::DrawState m_renderStates {};
	public:
		basic_ShapeItem(basic_Shape<B>* owner, ShapeGeometry geometry);
		void draw(cgss::RenderTarget& target) const override;
		void drawFast(cgss::RenderTarget& target) const override;
		void updateFromValue(const cgss::IntRect* rectangle) override;
	};

	template <class B>
	class basic_Shape :
		public Drawable<basic_Shape<B>, basic_ShapeItem<B>>,
		public Transformable {
	public:
		basic_Shape() = default;
		basic_Shape(basic_Shape&&) = default;
		basic_Shape(const basic_Shape&) = delete;
		basic_Shape& operator=(const basic_Shape&) = delete;
		basic_Shape& operator=(basic_Shape&&) = default;
		virtual ~basic_Shape() = default;

		void setRenderState(cgss::DrawState states);
		void setTexture(basic_Texture<B>* texture, bool resetRect = false);

		cgss::IntRect getRectangle() const;
		void setRectangle(cgss::IntRect rectangle);

		std::size_t getPointCount() const;
		void setPointCount(std::size_t pointCount);

		float getRadius() const;
		void setRadius(float radius);

		cgss::Point getPoint(std::size_t index) const;
		void setPoint(std::size_t index, float x, float y);

		cgss::Color getFillColor() const;
		void setFillColor(cgss::Color color);

		cgss::Color getOutlineColor() const;
		void setOutlineColor(cgss::Color color);

		float getOutlineThickness() const;
		void setOutlineThickness(float thickness);

		long getWidth() const;
		void setWidth(long width);

		long getHeight() const;
		void setHeight(long height);
	};

	namespace meta {
		template <class B>
		struct Log<basic_Shape<B>> {
			static constexpr auto classname = "Shape";
		};
	}
}
#endif
