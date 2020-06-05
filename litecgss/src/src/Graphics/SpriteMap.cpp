#include "SpriteMap.h"

cgss::SpriteMapItem::SpriteMapItem(SpriteMap* owner) {
}

void cgss::SpriteMapItem::draw(sf::RenderTarget& target) const {
  SpriteMapItem::drawFast(target);
}

void cgss::SpriteMapItem::drawFast(sf::RenderTarget& target) const {
  for (const auto& activableSprite: m_sprites) {
    if (activableSprite.active) {
      target.draw(activableSprite.sprite);
    }
  }
}

void cgss::SpriteMapItem::defineMap(unsigned long tileWidth, unsigned long tileCount) {
	m_tileWidth = tileWidth;
  if (tileCount == 0) {
    m_sprites = {};
		return;
	}

	m_sprites = std::vector<ActivableSprite>(tileCount);
}

void cgss::SpriteMap::defineMap(unsigned long tileWidth, unsigned long tileCount) {
  m_proxy->defineMap(tileWidth, tileCount);
}

void cgss::SpriteMap::setTile(std::size_t index, const sf::IntRect& tile, sf::Texture& texture) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
  }

  auto& sprite = m_proxy->m_sprites[index];
	sprite.sprite.setTexture(texture);
	sprite.sprite.setTextureRect(tile);
	sprite.active = true;
}

void cgss::SpriteMap::setTileRect(std::size_t index, const sf::IntRect& tile) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
  }
	m_proxy->m_sprites[index].sprite.setTextureRect(tile);
}

void cgss::SpriteMap::clear() {
  m_proxy->m_sprites.clear();
}

void cgss::SpriteMap::move(float x, float y) {
  unsigned long xIterator = static_cast<unsigned long>(x);
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite.setPosition(x, y);
		xIterator += m_proxy->m_tileWidth;
	}
}

void cgss::SpriteMap::moveOrigin(float ox, float oy) {
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite.setOrigin(ox, oy);
	}
}

void cgss::SpriteMap::setTileScale(float scale) {
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite.setScale(scale, scale);
	}
}
