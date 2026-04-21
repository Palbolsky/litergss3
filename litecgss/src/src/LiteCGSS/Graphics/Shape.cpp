#include <cassert>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include "Shape.h"
#include "LiteCGSS/Backend/SFMLBackend.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Common/Internal/SfColorConv.h"
#include "LiteCGSS/Common/Internal/SfRectConv.h"
#include "Texture.h"

cgss::ShapeItem::ShapeItem(Shape* owner, ShapeData shape) :
	m_shape(std::move(shape.second)),
	m_transformable(static_cast<Transformable*>(owner)),
	m_type(shape.first) {
	if (m_shape == nullptr) {
		throw std::runtime_error("Invalid shape provided");
	}
	m_shape->setFillColor(sf::Color::White);
}

void cgss::ShapeItem::draw(cgss::RenderTarget& target) const {
	sf::RenderStates states = m_renderStates.raw();
	states.transform *= cgss::backend::SFMLBackend::build_transform(*m_transformable);
	target.raw().draw(*m_shape, states);
}

void cgss::ShapeItem::drawFast(cgss::RenderTarget& target) const {
	sf::RenderStates states;
	states.transform = cgss::backend::SFMLBackend::build_transform(*m_transformable);
	target.raw().draw(*m_shape, states);
}

void cgss::Shape::setRenderState(cgss::DrawState states) {
	m_proxy->m_renderStates = std::move(states);
}

void cgss::Shape::setTexture(Texture* texture, bool resetRect) {
	m_proxy->m_linkedTexture = texture != nullptr ? texture->ref() : nullptr;
	if (m_proxy->m_linkedTexture != nullptr) {
		m_proxy->m_shape->setTexture(m_proxy->m_linkedTexture.get(), resetRect);
	}

	if (resetRect) {
		m_proxy->setValue(cgss::internal::fromSf(m_proxy->m_shape->getTextureRect()));
	}
}

cgss::IntRect cgss::Shape::getRectangle() const {
	return cgss::internal::fromSf(m_proxy->m_shape->getTextureRect());
}

void cgss::Shape::setRectangle(cgss::IntRect rectangle) {
	m_proxy->setValue(std::move(rectangle));
}

std::size_t cgss::Shape::getPointCount() const {
	return m_proxy->m_shape->getPointCount();
}

void cgss::Shape::setPointCount(std::size_t pointCount) const {
	switch (m_proxy->m_type) {
	case ShapeType::Circle:
		static_cast<sf::CircleShape&>(*m_proxy->m_shape).setPointCount(pointCount);
		break;
	case ShapeType::Convex:
		static_cast<sf::ConvexShape&>(*m_proxy->m_shape).setPointCount(pointCount);
		break;
	default:
		break;
	}
}

float cgss::Shape::getRadius() const {
	switch (m_proxy->m_type) {
	case ShapeType::Circle:
		return static_cast<sf::CircleShape&>(*m_proxy->m_shape).getRadius();
	default:
		return -1.F;
	}
}

void cgss::Shape::setRadius(float radius) {
	switch (m_proxy->m_type) {
	case ShapeType::Circle:
		static_cast<sf::CircleShape&>(*m_proxy->m_shape).setRadius(radius);
	default:
		break;
	}
}

sf::Vector2f cgss::Shape::getPoint(const std::size_t index) const {
	return m_proxy->m_shape->getPoint(index);
}

long cgss::Shape::getWidth() const {
	return m_proxy->m_shape->getGlobalBounds().width;
}

long cgss::Shape::getHeight() const {
	return m_proxy->m_shape->getGlobalBounds().height;
}

void cgss::Shape::setWidth(long width) {
	switch (m_proxy->m_type) {
		case ShapeType::Rectangle: {
			auto& rectangle = static_cast<sf::RectangleShape&>(*m_proxy->m_shape);
			auto size = rectangle.getSize();
			size.x = width;
			rectangle.setSize(size);
		}
		default:
			break;
	}
}

void cgss::Shape::setHeight(long height) {
	switch (m_proxy->m_type) {
		case ShapeType::Rectangle: {
			auto& rectangle = static_cast<sf::RectangleShape&>(*m_proxy->m_shape);
			auto size = rectangle.getSize();
			size.y = height;
			rectangle.setSize(size);
		}
		default:
			break;
	}
}

void cgss::Shape::setPoint(const std::size_t index, float x, float y) {
	switch (m_proxy->m_type) {
	case ShapeType::Convex:
		static_cast<sf::ConvexShape&>(*m_proxy->m_shape).setPoint(index, sf::Vector2f(x, y));
		break;
	default:
		break;
	}
}

cgss::Color cgss::Shape::getFillColor() const {
	return cgss::internal::fromSf(m_proxy->m_shape->getFillColor());
}

void cgss::Shape::setFillColor(cgss::Color color){
	return m_proxy->m_shape->setFillColor(cgss::internal::toSf(color));
}

cgss::Color cgss::Shape::getOutlineColor() const {
	return cgss::internal::fromSf(m_proxy->m_shape->getOutlineColor());
}

void cgss::Shape::setOutlineColor(cgss::Color color) {
	return m_proxy->m_shape->setOutlineColor(cgss::internal::toSf(color));
}

float cgss::Shape::getOutlineThickness() const {
	return m_proxy->m_shape->getOutlineThickness();
}

void cgss::Shape::setOutlineThickness(float thickness) const {
	return m_proxy->m_shape->setOutlineThickness(thickness);
}

void cgss::ShapeItem::updateFromValue(const cgss::IntRect* rectangle) {
	m_shape->setTextureRect(rectangle == nullptr ? sf::IntRect{} : cgss::internal::toSf(*rectangle));
}
