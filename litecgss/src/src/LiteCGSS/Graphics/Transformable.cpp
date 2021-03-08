#include "Transformable.h"

cgss::Transformable::Transformable(sf::Transformable& instance) :
	m_instance(&instance) {
}

void cgss::Transformable::setInstance(sf::Transformable& instance) {
	m_instance = &instance;
}

float cgss::Transformable::getX() const {
	return m_instance->getPosition().x;
}

float cgss::Transformable::getY() const {
	return m_instance->getPosition().y;
}

float cgss::Transformable::getOx() const {
	return m_instance->getOrigin().x;
}

float cgss::Transformable::getOy() const {
	return m_instance->getOrigin().y;
}

float cgss::Transformable::getAngle() const {
	return -m_instance->getRotation();
}

float cgss::Transformable::getScaleX() const {
	return m_instance->getScale().x;
}

float cgss::Transformable::getScaleY() const {
	return m_instance->getScale().y;
}

void cgss::Transformable::move(float x, float y) {
	m_instance->setPosition(x, y);
}

void cgss::Transformable::moveOrigin(float ox, float oy) {
	m_instance->setOrigin(ox, oy);
}

void cgss::Transformable::setAngle(float angle) {
	m_instance->setRotation(-angle);
}

void cgss::Transformable::scale(float xScale, float yScale) {
	m_instance->setScale(xScale, yScale);
}
