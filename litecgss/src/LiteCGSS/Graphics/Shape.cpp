#include "LiteCGSS/Graphics/Shape.h"

#include <cassert>

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Graphics/Texture.h"

template <class B>
cgss::basic_ShapeItem<B>::basic_ShapeItem(basic_Shape<B>* owner, ShapeGeometry geometry) :
	m_geometry(std::move(geometry)),
	m_transformable(static_cast<Transformable*>(owner)) {
}

template <class B>
void cgss::basic_ShapeItem<B>::draw(cgss::RenderTarget& target) const {
	using cgss::backend::native_of;
	const auto& base = native_of(m_renderStates);
	B::Ops::draw_shape_transformed(native_of(target), m_geometry, m_linkedTexture.get(), *m_transformable, &base);
}

template <class B>
void cgss::basic_ShapeItem<B>::drawFast(cgss::RenderTarget& target) const {
	using cgss::backend::native_of;
	B::Ops::draw_shape_transformed(native_of(target), m_geometry, m_linkedTexture.get(), *m_transformable, nullptr);
}

template <class B>
void cgss::basic_ShapeItem<B>::updateFromValue(const cgss::IntRect* rectangle) {
	m_geometry.textureRect = rectangle == nullptr ? cgss::IntRect{ 0, 0, 0, 0 } : *rectangle;
}

template <class B>
void cgss::basic_Shape<B>::setRenderState(cgss::DrawState states) {
	this->m_proxy->m_renderStates = std::move(states);
}

template <class B>
void cgss::basic_Shape<B>::setTexture(basic_Texture<B>* texture, bool resetRect) {
	this->m_proxy->m_linkedTexture = texture != nullptr ? texture->ref() : nullptr;
	if (resetRect && this->m_proxy->m_linkedTexture != nullptr) {
		const auto sz = B::Ops::get_texture_size(*this->m_proxy->m_linkedTexture);
		this->m_proxy->setValue(cgss::IntRect{ 0, 0, static_cast<int>(sz.x), static_cast<int>(sz.y) });
	}
}

template <class B>
cgss::IntRect cgss::basic_Shape<B>::getRectangle() const {
	return this->m_proxy->m_geometry.textureRect;
}

template <class B>
void cgss::basic_Shape<B>::setRectangle(cgss::IntRect rectangle) {
	this->m_proxy->setValue(std::move(rectangle));
}

template <class B>
std::size_t cgss::basic_Shape<B>::getPointCount() const {
	const auto& g = this->m_proxy->m_geometry;
	switch (g.type) {
		case ShapeType::Rectangle: return 4;
		case ShapeType::Circle:    return g.pointCount;
		case ShapeType::Convex:    return g.points.size();
	}
	return 0;
}

template <class B>
void cgss::basic_Shape<B>::setPointCount(std::size_t pointCount) {
	auto& g = this->m_proxy->m_geometry;
	switch (g.type) {
		case ShapeType::Circle:
			g.pointCount = pointCount;
			break;
		case ShapeType::Convex:
			g.pointCount = pointCount;
			g.points.resize(pointCount);
			break;
		default:
			break;
	}
}

template <class B>
float cgss::basic_Shape<B>::getRadius() const {
	const auto& g = this->m_proxy->m_geometry;
	return g.type == ShapeType::Circle ? g.radius : -1.f;
}

template <class B>
void cgss::basic_Shape<B>::setRadius(float radius) {
	auto& g = this->m_proxy->m_geometry;
	if (g.type == ShapeType::Circle) {
		g.radius = radius;
	}
}

template <class B>
cgss::Point cgss::basic_Shape<B>::getPoint(std::size_t index) const {
	return B::Ops::get_shape_point(this->m_proxy->m_geometry, index);
}

template <class B>
void cgss::basic_Shape<B>::setPoint(std::size_t index, float x, float y) {
	auto& g = this->m_proxy->m_geometry;
	if (g.type == ShapeType::Convex && index < g.points.size()) {
		g.points[index] = { x, y };
	}
}

template <class B>
long cgss::basic_Shape<B>::getWidth() const {
	const auto b = B::Ops::get_shape_global_bounds(this->m_proxy->m_geometry, *this);
	return static_cast<long>(b.width);
}

template <class B>
long cgss::basic_Shape<B>::getHeight() const {
	const auto b = B::Ops::get_shape_global_bounds(this->m_proxy->m_geometry, *this);
	return static_cast<long>(b.height);
}

template <class B>
void cgss::basic_Shape<B>::setWidth(long width) {
	auto& g = this->m_proxy->m_geometry;
	if (g.type == ShapeType::Rectangle) {
		g.width = static_cast<float>(width);
	}
}

template <class B>
void cgss::basic_Shape<B>::setHeight(long height) {
	auto& g = this->m_proxy->m_geometry;
	if (g.type == ShapeType::Rectangle) {
		g.height = static_cast<float>(height);
	}
}

template <class B>
cgss::Color cgss::basic_Shape<B>::getFillColor() const {
	return this->m_proxy->m_geometry.fillColor;
}

template <class B>
void cgss::basic_Shape<B>::setFillColor(cgss::Color color) {
	this->m_proxy->m_geometry.fillColor = color;
}

template <class B>
cgss::Color cgss::basic_Shape<B>::getOutlineColor() const {
	return this->m_proxy->m_geometry.outlineColor;
}

template <class B>
void cgss::basic_Shape<B>::setOutlineColor(cgss::Color color) {
	this->m_proxy->m_geometry.outlineColor = color;
}

template <class B>
float cgss::basic_Shape<B>::getOutlineThickness() const {
	return this->m_proxy->m_geometry.outlineThickness;
}

template <class B>
void cgss::basic_Shape<B>::setOutlineThickness(float thickness) {
	this->m_proxy->m_geometry.outlineThickness = thickness;
}

template class cgss::basic_ShapeItem<cgss::backend::ActiveBackend>;
template class cgss::basic_Shape<cgss::backend::ActiveBackend>;
