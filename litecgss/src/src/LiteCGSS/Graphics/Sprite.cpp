#include "Sprite.h"

#include <SFML/Graphics/Sprite.hpp>

#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Common/Meta/metadata.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "RenderStates.h"
#include "Texture.h"

template <class B>
cgss::basic_SpriteItem<B>::basic_SpriteItem(basic_Sprite<B>* owner) :
	m_sprite(std::make_unique<typename B::native_sprite>()),
	m_transformable(static_cast<Transformable*>(owner)) {
}

template <class B>
cgss::basic_SpriteItem<B>::~basic_SpriteItem() = default;

template <class B>
void cgss::basic_SpriteItem<B>::draw(cgss::RenderTarget& target) const {
	basic_SpriteItem::drawFast(target);
}

template <class B>
void cgss::basic_SpriteItem<B>::updateFromValue(const RenderStatesData*) {}

template <class B>
void cgss::basic_SpriteItem<B>::drawFast(cgss::RenderTarget& target) const {
	using cgss::backend::native_of;
	const auto* base = ShaderRenderable::getValue() == nullptr
		? nullptr
		: &native_of(ShaderRenderable::getValue()->getRenderStates());
	B::Ops::draw_sprite_transformed(native_of(target), *m_sprite, *m_transformable, base);
}

template <class B>
void cgss::basic_Sprite<B>::bindRenderStates(RenderStates* renderStates) {
	if (renderStates != nullptr) {
		renderStates->data().setShaderUniform("texture", cgss::CurrentTexture);
		this->m_proxy->ShaderRenderable::bindValue(&renderStates->data());
	} else {
		this->m_proxy->ShaderRenderable::bindValue(nullptr);
	}
}

template <class B>
void cgss::basic_Sprite<B>::setMirror(bool mirror) {
	if (this->m_proxy->m_mirror ^ mirror) {
		horizontalFlip();
		this->m_proxy->m_mirror = mirror;
	}
}

template <class B>
void cgss::basic_Sprite<B>::horizontalFlip() {
	auto rect = B::Ops::get_sprite_texture_rect(*this->m_proxy->m_sprite);
	rect.left += rect.width;
	rect.width = -rect.width;
	setTextureRect(rect);
}

template <class B>
cgss::Color cgss::basic_Sprite<B>::getColor() const {
	return B::Ops::get_sprite_color(*this->m_proxy->m_sprite);
}

template <class B>
float cgss::basic_Sprite<B>::getX() const {
	return this->m_proxy->m_wantedX;
}

template <class B>
float cgss::basic_Sprite<B>::getY() const {
	return this->m_proxy->m_wantedY;
}

template <class B>
void cgss::basic_Sprite<B>::move(float x, float y) {
	this->m_proxy->m_wantedX = x;
	this->m_proxy->m_wantedY = y;
	Transformable::move(x, y);
}

template <class B>
void cgss::basic_Sprite<B>::setColor(cgss::Color color) {
	B::Ops::set_sprite_color(*this->m_proxy->m_sprite, color);
}

template <class B>
void cgss::basic_SpriteItem<B>::setOpacity(std::uint8_t opacity) {
	auto color = B::Ops::get_sprite_color(*m_sprite);
	color.a = opacity;
	B::Ops::set_sprite_color(*m_sprite, color);
}

template <class B>
void cgss::basic_Sprite<B>::setTexture(basic_Texture<B>& texture, bool resetRect) {
	this->m_proxy->m_linkedTexture = texture.ref();
	B::Ops::set_sprite_texture(*this->m_proxy->m_sprite, *this->m_proxy->m_linkedTexture, resetRect);
	if (resetRect) {
		this->m_proxy->GraphicsStackItem::setValue(B::Ops::get_sprite_texture_rect(*this->m_proxy->m_sprite));
	}
}

template <class B>
void cgss::basic_SpriteItem<B>::updateFromValue(const cgss::IntRect* rectangle) {
	const auto textureSize = B::Ops::get_sprite_bound_texture_size(*m_sprite);
	auto tmpRectangle = rectangle == nullptr
		? cgss::IntRect{0, 0, static_cast<int>(textureSize.x), static_cast<int>(textureSize.y)}
		: *rectangle;
	if (m_mirror) {
		tmpRectangle.left += tmpRectangle.width;
		tmpRectangle.width = -tmpRectangle.width;
	}
	B::Ops::set_sprite_texture_rect(*m_sprite, tmpRectangle);
}

template <class B>
void cgss::basic_Sprite<B>::setTextureRect(cgss::IntRect rectangle) {
	this->m_proxy->GraphicsStackItem::setValue(std::move(rectangle));
}

template <class B>
cgss::IntRect cgss::basic_Sprite<B>::getTextureRect() const {
	return B::Ops::get_sprite_texture_rect(*this->m_proxy->m_sprite);
}

template class cgss::basic_SpriteItem<cgss::backend::ActiveBackend>;
template class cgss::basic_Sprite<cgss::backend::ActiveBackend>;
