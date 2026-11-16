// SFML implementation of cgss::ViewportItem::takeSnapshot. The shared bulk
// of Viewport's body lives in Views/Viewport.cpp; this one method is split
// out per-backend because it uses local sf::RenderTexture + sf::Sprite for
// the off-screen render-and-grab choreography. Raylib's equivalent uses
// BeginTextureMode/EndTextureMode + LoadImageFromTexture — different
// model entirely. Selected by CMake when CGSS_BACKEND=sfml.

#include <SFML/Graphics/Color.hpp>
#include "Common/Profiling.h"
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Views/Viewport.h"

std::unique_ptr<cgss::Texture> cgss::ViewportItem::takeSnapshot() const {
	CGSS_PROFILE_ZONE("cgss::ViewportItem::takeSnapshot");
	using Ops = cgss::backend::ActiveBackend::Ops;
	using cgss::backend::native_of;

	sf::RenderTexture render;
	sf::Sprite renderSprite;
	cgss::Camera renderView = ViewBox::clone();
	cgss::RenderTarget renderTarget{render};
	render.create(ViewBox::getWidth(), ViewBox::getHeight());
	Ops::view_set_viewport(native_of(renderView), 0.f, 0.f, 1.f, 1.f);
	render.setView(native_of(renderView));

	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates) {
		// Clearing render
		render.clear(sf::Color::Transparent);

		// Drawing sprites
		drawables().drawFast(renderTarget);
		render.display();

		sf::Texture copy = sf::Texture(render.getTexture());
		renderSprite.setTexture(copy);
		renderSprite.setPosition(0, 0);

		// Draw the result
		render.clear(sf::Color::Transparent);
		render.draw(renderSprite, native_of(*renderStates));
		render.display();
	} else {
		render.clear(sf::Color::Transparent);
		drawables().drawFast(renderTarget);
		render.display();
	}

	return std::make_unique<cgss::Texture>(cgss::Texture::create_from_native(sf::Texture(render.getTexture())));
}
