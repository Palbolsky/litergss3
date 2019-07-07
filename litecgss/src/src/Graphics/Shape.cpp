#include "Shape.h"

void cgss::Shape::draw(sf::RenderTarget& target) const {
	if (m_renderStates != nullptr) {
		if (m_shape != nullptr) {
			target.draw(*m_shape, *m_renderStates);
		}
	} else {
		Shape::drawFast(target);
	}
}

void cgss::Shape::drawFast(sf::RenderTarget& target) const {
	if (m_shape != nullptr) {
		target.draw(*m_shape);
	}
}
void cgss::Shape::setRenderState(sf::RenderStates* states) {
	m_renderStates = states;
}