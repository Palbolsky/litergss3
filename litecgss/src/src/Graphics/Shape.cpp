#include "Shape.h"
#include "Common/Rectangle.h"
#include "Texture.h"

cgss::Shape::Shape(ShapeData shape) :
	Transformable(shape.second == nullptr ? m_defaultShape : *shape.second),
	m_shape(std::move(shape.second)),
	m_type(shape.first) {
	if (m_shape == nullptr) {
		throw std::runtime_error("Invalid shape provided");
	}
}

void cgss::Shape::draw(sf::RenderTarget& target) const {
	target.draw(*m_shape, m_renderStates);	
}

void cgss::Shape::drawFast(sf::RenderTarget& target) const {
	target.draw(*m_shape);	
}

void cgss::Shape::setRenderState(sf::RenderStates states) {
	m_renderStates = std::move(states);
}

void cgss::Shape::linkTexture(Texture* texture) {
	// /!\ Here, we assume the Texture has a longer lifetime than our Shape
	m_linkedTexture = texture;
	if (m_linkedTexture != nullptr) {
		texture->linkShape(*m_shape);
	}
}

sf::IntRect cgss::Shape::getRectangle() const {
	return m_shape->getTextureRect();
}

void cgss::Shape::setRectangle(sf::IntRect rectangle) {
	m_shape->setTextureRect(rectangle);
}

std::size_t cgss::Shape::getPointCount() const {
	return m_shape->getPointCount();
}

void cgss::Shape::setPointCount(std::size_t pointCount) const {
	switch (m_type) {
	case ShapeType::Circle:
		static_cast<sf::CircleShape&>(*m_shape).setPointCount(pointCount);
		break;
	case ShapeType::Convex:
		static_cast<sf::ConvexShape&>(*m_shape).setPointCount(pointCount);
		break;
	default:
		break;
	}
}

float cgss::Shape::getRadius() const {
	switch (m_type) {
	case ShapeType::Circle:
		return static_cast<sf::CircleShape&>(*m_shape).getRadius();
	default:
		return -1.F;
	}
}

void cgss::Shape::setRadius(float radius) {
	switch (m_type) {
	case ShapeType::Circle:
		static_cast<sf::CircleShape&>(*m_shape).setRadius(radius);
	default:
		break;
	}
}

sf::Vector2f cgss::Shape::getPoint(const std::size_t index) const {
	return m_shape->getPoint(index);
}

void cgss::Shape::setPoint(const std::size_t index, float x, float y) {
	switch (m_type) {
	case ShapeType::Convex:
		static_cast<sf::ConvexShape&>(*m_shape).setPoint(index, sf::Vector2f(x, y));
		break;
	default:
		break;
	}
}

sf::Color cgss::Shape::getFillColor() const {
	return m_shape->getFillColor();
}

void cgss::Shape::setFillColor(sf::Color color){
	return m_shape->setFillColor(color);
}

sf::Color cgss::Shape::getOutlineColor() const {
	return m_shape->getOutlineColor();
}

void cgss::Shape::setOutlineColor(sf::Color color) {
	return m_shape->setOutlineColor(color);
}

float cgss::Shape::getOutlineThickness() const {
	return m_shape->getOutlineThickness();
}

void cgss::Shape::setOutlineThickness(float thickness) const {
	return m_shape->setOutlineThickness(thickness);
}
