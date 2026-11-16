// Tile-grid sprite rendering. Tiles are owned as raw `B::native_sprite`
// handles (NOT cgss::Sprite) — they have no per-tile transformable state,
// just per-tile texture-rect + position + active flag. Body routes through
// `B::Ops::set_sprite_*` / `sprite_set_*` / `draw_sprite` for backend
// portability.
#include "LiteCGSS/Graphics/SpriteMap.h"
#include "Backend/ActiveBackend.h"
#include "Graphics/LoggerDrawables.h"

SKA_LOGC_CONFIG(ska::LogLevel::Error, cgss::SpriteMapItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Debug, cgss::SpriteMapItem)
#define LOG_INFO  SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Info,  cgss::SpriteMapItem)
#define LOG_WARN  SKA_LOGC_STATIC(GetLoggerDrawables(), ska::LogLevel::Warn,  cgss::SpriteMapItem)

namespace { using Ops = cgss::backend::ActiveBackend::Ops; }

cgss::SpriteMapItem::ActivableSprite::ActivableSprite() :
	sprite(std::make_unique<typename cgss::backend::ActiveBackend::native_sprite>()) {
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
			Ops::draw_sprite(cgss::backend::native_of(target), *activableSprite.sprite);
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

void cgss::SpriteMap::setTile(std::size_t index, const cgss::IntRect& tile, cgss::Texture& texture) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
	}
	LOG_DEBUG << "Activate tile index = " << index << " tileRect = " << tile.left << ", " << tile.top << ", " << tile.width << ", " << tile.height;
	auto& sprite = m_proxy->m_sprites[index];
	// Take a shared_ptr ref on the native texture so the GPU handle survives
	// the cgss::Texture wrapper going out of scope (callers commonly pass a
	// temporary or a cache slot). set_sprite_texture itself only stores a
	// non-owning pointer; this shared_ptr is the owning anchor.
	sprite.linkedTexture = Ops::texture_share(texture);
	Ops::set_sprite_texture(*sprite.sprite, *sprite.linkedTexture, /*resetRect=*/false);
	Ops::set_sprite_texture_rect(*sprite.sprite, tile);
	sprite.active = true;
}

void cgss::SpriteMap::setTileRect(std::size_t index, const cgss::IntRect& tile) {
	if (index >= m_proxy->m_sprites.size()) {
		return;
	}
	Ops::set_sprite_texture_rect(*m_proxy->m_sprites[index].sprite, tile);
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
		Ops::sprite_set_position(*sprite.sprite, xIterator, y);
		xIterator += static_cast<float>(m_proxy->m_tileWidth);
	}
}

void cgss::SpriteMap::moveOrigin(float ox, float oy) {
	LOG_DEBUG << "Settings origin = " << ox << ", " << oy;
	for (auto& sprite : m_proxy->m_sprites) {
		Ops::sprite_set_origin(*sprite.sprite, ox, oy);
	}
}

void cgss::SpriteMap::setTileScale(float scale) {
	LOG_WARN << "Settings scale = " << scale;
	for (auto& sprite : m_proxy->m_sprites) {
		Ops::sprite_set_scale(*sprite.sprite, scale, scale);
	}
}
