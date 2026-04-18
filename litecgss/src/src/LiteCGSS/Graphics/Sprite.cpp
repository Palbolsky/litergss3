#include <cassert>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "Sprite.h"
#include "RenderStates.h"
#include "Texture.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Common/Internal/SfColorConv.h"
#include "LiteCGSS/Common/Internal/SfRectConv.h"
#include "LiteCGSS/Common/Meta/metadata.h"

cgss::SpriteItem::SpriteItem(Sprite* owner) :
	m_sprite(std::make_unique<sf::Sprite>()) {
	owner->setInstance(*m_sprite);
}

cgss::SpriteItem::~SpriteItem() = default;

void cgss::SpriteItem::draw(cgss::RenderTarget& target) const {
	SpriteItem::drawFast(target);
}

void cgss::SpriteItem::updateFromValue(const RenderStatesData*) {}

void cgss::SpriteItem::drawFast(cgss::RenderTarget& target) const {
	target.raw().draw(*m_sprite, ShaderRenderable::getValue() == nullptr ? sf::RenderStates::Default : ShaderRenderable::getValue()->getRenderStates().raw());
}

void cgss::Sprite::bindRenderStates(RenderStates* renderStates) {
	if (renderStates != nullptr) {
		renderStates->data().setShaderUniform("texture", cgss::CurrentTexture);
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
	auto rect = m_proxy->m_sprite->getTextureRect();
	rect.left += rect.width;
	rect.width = -rect.width;
	setTextureRect(cgss::internal::fromSf(rect));
}

cgss::Color cgss::Sprite::getColor() const {
	return cgss::internal::fromSf(m_proxy->m_sprite->getColor());
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

void cgss::Sprite::setColor(cgss::Color color) {
	m_proxy->m_sprite->setColor(cgss::internal::toSf(color));
}

void cgss::SpriteItem::setOpacity(std::uint8_t opacity) {
	sf::Color col = m_sprite->getColor();
	col.a = opacity;
	m_sprite->setColor(col);
}

void cgss::Sprite::setTexture(Texture& texture, bool resetRect) {
	m_proxy->m_linkedTexture = texture.ref();
	m_proxy->m_sprite->setTexture(*m_proxy->m_linkedTexture.get(), resetRect);
	if (resetRect) {
		m_proxy->GraphicsStackItem::setValue(cgss::internal::fromSf(m_proxy->m_sprite->getTextureRect()));
	}
}

void cgss::SpriteItem::updateFromValue(const cgss::IntRect* rectangle) {
	auto textureSize = m_sprite->getTexture() == nullptr ? sf::Vector2u{} : m_sprite->getTexture()->getSize();
	auto tmpRectangle = rectangle == nullptr
		? cgss::IntRect{0, 0, static_cast<int>(textureSize.x), static_cast<int>(textureSize.y)}
		: *rectangle;
	if (m_mirror) {
		tmpRectangle.left += tmpRectangle.width;
		tmpRectangle.width = -tmpRectangle.width;
	}
	m_sprite->setTextureRect(cgss::internal::toSf(tmpRectangle));
}

void cgss::Sprite::setTextureRect(cgss::IntRect rectangle) {
	m_proxy->GraphicsStackItem::setValue(std::move(rectangle));
}

cgss::IntRect cgss::Sprite::getTextureRect() const {
	return cgss::internal::fromSf(m_proxy->m_sprite->getTextureRect());
}
