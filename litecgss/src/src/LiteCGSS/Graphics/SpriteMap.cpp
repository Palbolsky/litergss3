#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "SpriteMap.h"
#include "LiteCGSS/Common/Internal/SfRectConv.h"
#include "LoggerDrawables.h"

SKA_LOGC_CONFIG(ska::LogLevel::Error, cgss::SpriteMapItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::SpriteMapItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info, cgss::SpriteMapItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn, cgss::SpriteMapItem)

cgss::SpriteMapItem::ActivableSprite::ActivableSprite() :
	sprite(std::make_unique<sf::Sprite>()) {
}
cgss::SpriteMapItem::ActivableSprite::~ActivableSprite() = default;
cgss::SpriteMapItem::ActivableSprite::ActivableSprite(ActivableSprite&&) noexcept = default;
cgss::SpriteMapItem::ActivableSprite& cgss::SpriteMapItem::ActivableSprite::operator=(ActivableSprite&&) noexcept = default;

cgss::SpriteMapItem::SpriteMapItem(SpriteMap*) {
}

void cgss::SpriteMapItem::draw(cgss::RenderTarget& target) const {
	SpriteMapItem::drawFast(target);
}

void cgss::SpriteMapItem::drawFast(cgss::RenderTarget& target) const {
	LOG_INFO << "Drawing " << m_sprites.size() << " sprites";
	for (const auto& activableSprite: m_sprites) {
		if (activableSprite.active) {
			target.raw().draw(*activableSprite.sprite);
		}
	}
}

void cgss::SpriteMapItem::defineMap(unsigned long tileWidth, unsigned long tileCount) {
	m_tileWidth = tileWidth;
	LOG_WARN << "Defining SpriteMap tileWidth = " << tileWidth << " tileCount = " << tileCount;

	if (tileCount == 0) {
		m_sprites.clear();
		return;
	}

	m_sprites = std::vector<ActivableSprite>(tileCount);
}

void cgss::SpriteMap::defineMap(unsigned long tileWidth, unsigned long tileCount) {
	m_proxy->defineMap(tileWidth, tileCount);
}

void cgss::SpriteMap::setTile(std::size_t index, const cgss::IntRect& tile, sf::Texture& texture) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
	}
	LOG_DEBUG << "Activate tile index = " << index << " tileRect = " << tile.left << ", " << tile.top << ", " << tile.width << ", " << tile.height;
	auto& sprite = m_proxy->m_sprites[index];
	sprite.sprite->setTexture(texture);
	sprite.sprite->setTextureRect(cgss::internal::toSf(tile));
	sprite.active = true;
}

void cgss::SpriteMap::setTileRect(std::size_t index, const cgss::IntRect& tile) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
	}
	m_proxy->m_sprites[index].sprite->setTextureRect(cgss::internal::toSf(tile));
}

void cgss::SpriteMap::reset() {
	LOG_DEBUG << "SpriteMap resetted";
		for (auto& sprite : m_proxy->m_sprites) {
		sprite.active = false;
	}
}

void cgss::SpriteMap::move(float x, float y) {
	LOG_WARN << "Moving = " << x << ", " << y;
	auto xIterator = x;
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite->setPosition(xIterator, y);
		xIterator += static_cast<float>(m_proxy->m_tileWidth);
	}
}

void cgss::SpriteMap::moveOrigin(float ox, float oy) {
	LOG_DEBUG << "Settings origin = " << ox << ", " << oy;
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite->setOrigin(ox, oy);
	}
}

void cgss::SpriteMap::setTileScale(float scale) {
	LOG_WARN << "Settings scale = " << scale;
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.sprite->setScale(scale, scale);
	}
}
