// Tile-grid sprite rendering. Tiles are owned as raw `B::native_sprite`
// handles (NOT cgss::Sprite) — they have no per-tile transformable state,
// just per-tile texture-rect + position + active flag. Body routes through
// `B::Ops::set_sprite_*` / `sprite_set_*` / `draw_sprite` for backend
// portability.
#include "LiteCGSS/Graphics/SpriteMap.h"
#include "Common/Profiling.h"

#include <algorithm>

#include "Backend/ActiveBackend.h"
#include "Common/Log.h"

CGSS_LOG_COMPONENT("SpriteMap", err)


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
	CGSS_PROFILE_ZONE("cgss::SpriteMapItem::drawFast");
	CGSS_LOG_INFO("Drawing {} sprites in {} texture groups", m_sprites.size(), m_textureGroups.size());
	// One batch per texture (tileset): tiles don't overlap, so the order
	// between groups doesn't show.
	for (const auto& group : m_textureGroups) {
		m_batch.clear();
		for (const auto index : group.spriteIndices) {
			m_batch.push_back(m_sprites[index].sprite.get());
		}
		Ops::draw_sprite_batch(cgss::backend::native_of(target), m_batch.data(), m_batch.size());
	}
}

void cgss::SpriteMapItem::addToTextureGroup(std::size_t index, const void* texture) {
	const auto group = std::find_if(m_textureGroups.begin(), m_textureGroups.end(),
		[texture](const auto& g) { return g.texture == texture; });
	if (group == m_textureGroups.end()) {
		m_textureGroups.push_back({ texture, { index } });
	} else {
		group->spriteIndices.push_back(index);
	}
}

void cgss::SpriteMapItem::removeFromTextureGroup(std::size_t index, const void* texture) {
	const auto group = std::find_if(m_textureGroups.begin(), m_textureGroups.end(),
		[texture](const auto& g) { return g.texture == texture; });
	if (group == m_textureGroups.end()) {
		return;
	}
	auto& indices = group->spriteIndices;
	indices.erase(std::remove(indices.begin(), indices.end(), index), indices.end());
	if (indices.empty()) {
		m_textureGroups.erase(group);
	}
}

void cgss::SpriteMapItem::defineMap(unsigned long tileWidth, unsigned long tileCount) {
	m_tileWidth = tileWidth;
	CGSS_LOG_WARN("Defining SpriteMap tileWidth = {} tileCount = {}", tileWidth, tileCount);

	m_textureGroups.clear();
	if (tileCount == 0) {
		m_sprites.clear();
		return;
	}

	m_sprites = std::vector<ActivableSprite>(tileCount);
	m_batch.reserve(tileCount);
}

void cgss::SpriteMap::defineMap(unsigned long tileWidth, unsigned long tileCount) {
	m_proxy->defineMap(tileWidth, tileCount);
}

void cgss::SpriteMap::setTile(std::size_t index, const cgss::IntRect& tile, cgss::Texture& texture) {
	CGSS_PROFILE_ZONE("cgss::SpriteMap::setTile");
	if (index >= m_proxy->m_sprites.size()) {
		return;
	}
	CGSS_LOG_DEBUG("Activate tile index = {} tileRect = {}, {}, {}, {}", index, tile.left, tile.top, tile.width, tile.height);
	auto& sprite = m_proxy->m_sprites[index];
	// Take a shared_ptr ref on the native texture so the GPU handle survives
	// the cgss::Texture wrapper going out of scope (callers commonly pass a
	// temporary or a cache slot). set_sprite_texture itself only stores a
	// non-owning pointer; this shared_ptr is the owning anchor.
	auto linkedTexture = Ops::texture_share(texture);
	const void* previousTexture = sprite.active ? sprite.linkedTexture.get() : nullptr;
	if (previousTexture != linkedTexture.get()) {
		if (previousTexture != nullptr) {
			m_proxy->removeFromTextureGroup(index, previousTexture);
		}
		m_proxy->addToTextureGroup(index, linkedTexture.get());
	}
	sprite.linkedTexture = std::move(linkedTexture);
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
	CGSS_LOG_DEBUG("SpriteMap resetted");
	for (auto& sprite : m_proxy->m_sprites) {
		sprite.active = false;
	}
	m_proxy->m_textureGroups.clear();
}

void cgss::SpriteMap::move(float x, float y) {
	CGSS_LOG_WARN("Moving = {}, {}", x, y);
	auto xIterator = x;
	for (auto& sprite : m_proxy->m_sprites) {
		Ops::sprite_set_position(*sprite.sprite, xIterator, y);
		xIterator += static_cast<float>(m_proxy->m_tileWidth);
	}
}

void cgss::SpriteMap::moveOrigin(float ox, float oy) {
	CGSS_LOG_DEBUG("Settings origin = {}, {}", ox, oy);
	for (auto& sprite : m_proxy->m_sprites) {
		Ops::sprite_set_origin(*sprite.sprite, ox, oy);
	}
}

void cgss::SpriteMap::setTileScale(float scale) {
	CGSS_LOG_WARN("Settings scale = {}", scale);
	for (auto& sprite : m_proxy->m_sprites) {
		Ops::sprite_set_scale(*sprite.sprite, scale, scale);
	}
}
