#include <cassert>
#include "Sprite.h"
#include "RenderStates.h"
#include "Texture.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Common/Meta/metadata.h"

cgss::SpriteItem::SpriteItem(Sprite* owner) {
	owner->setInstance(m_sprite);
}

void cgss::SpriteItem::draw(sf::RenderTarget& target) const {
	SpriteItem::drawFast(target);
}

void cgss::SpriteItem::updateFromValue(const RenderStatesData*) {}

void cgss::SpriteItem::drawFast(sf::RenderTarget& target) const {
	target.draw(m_sprite, ShaderRenderable::getValue() == nullptr ? sf::RenderStates::Default : ShaderRenderable::getValue()->getRenderStates());
}

void cgss::Sprite::bindRenderStates(RenderStates* renderStates) {
	if (renderStates != nullptr) {
		renderStates->data().setShaderUniform("texture", sf::Shader::CurrentTexture);
		m_proxy->ShaderRenderable::bindValue(&renderStates->data());
	} else {
		m_proxy->ShaderRenderable::bindValue(nullptr);
	}
}

void cgss::Sprite::setMirror(bool mirror) {
	if (m_proxy->m_mirror ^ mirror) {
		horizontalFlip();
		m_proxy->m_mirror = mirror;
	}
}

void cgss::Sprite::horizontalFlip() {
	auto rect = m_proxy->m_sprite.getTextureRect();
	rect.left += rect.width;
	rect.width = -rect.width;
	setTextureRect(rect);
}

sf::Color cgss::Sprite::getColor() const {
	return m_proxy->m_sprite.getColor();
}

float cgss::Sprite::getX() const {
	return m_proxy->m_wantedX;
}

float cgss::Sprite::getY() const {
	return m_proxy->m_wantedY;
}

void cgss::Sprite::move(float x, float y) {
	m_proxy->m_wantedX = x;
	m_proxy->m_wantedY = y;
	Transformable::move(x, y);
}

void cgss::Sprite::setColor(sf::Color color) {
	m_proxy->m_sprite.setColor(std::move(color));
}

void cgss::SpriteItem::setOpacity(std::uint8_t opacity) {
	sf::Color col = m_sprite.getColor();
	col.a = opacity;
	m_sprite.setColor(col);
}

void cgss::Sprite::setTexture(Texture& texture, bool resetRect) {
	m_proxy->m_linkedTexture = texture.ref();
	m_proxy->m_sprite.setTexture(*m_proxy->m_linkedTexture.get(), resetRect);
	if (resetRect) {
		m_proxy->GraphicsStackItem::setValue(m_proxy->m_sprite.getTextureRect());
	}
}

void cgss::SpriteItem::updateFromValue(const sf::IntRect* rectangle) {
	auto textureSize = m_sprite.getTexture() == nullptr ? sf::Vector2u{} : m_sprite.getTexture()->getSize();
	auto tmpRectangle = rectangle == nullptr ? sf::IntRect{0, 0, static_cast<int>(textureSize.x), static_cast<int>(textureSize.y) } : *rectangle;
	if (m_mirror) {
		tmpRectangle.left += tmpRectangle.width;
		tmpRectangle.width = -tmpRectangle.width;
	}
	m_sprite.setTextureRect(std::move(tmpRectangle));
}

void cgss::Sprite::setTextureRect(sf::IntRect rectangle) {
	m_proxy->GraphicsStackItem::setValue(std::move(rectangle));
}

const sf::IntRect& cgss::Sprite::getTextureRect() const {
	return m_proxy->m_sprite.getTextureRect();
}
