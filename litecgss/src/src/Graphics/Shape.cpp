#include "Shape.h"

void lr::Shape::draw(sf::RenderTarget& target) const {
	if (m_renderStates != nullptr) {
		if (m_shape != nullptr) {
			target.draw(*m_shape, *m_renderStates);
		}
	} else {
		Shape::drawFast(target);
	}
}

void lr::Shape::drawFast(sf::RenderTarget& target) const {
	if (m_shape != nullptr) {
		target.draw(*m_shape);
	}
}
void lr::Shape::setRenderState(sf::RenderStates* states) {
	m_renderStates = states;
}