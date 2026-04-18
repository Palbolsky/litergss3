#include "DrawState.h"
#include <SFML/Graphics/RenderStates.hpp>

cgss::DrawState::DrawState() : m_raw(std::make_unique<sf::RenderStates>()) {}

cgss::DrawState::DrawState(const sf::RenderStates& raw) : m_raw(std::make_unique<sf::RenderStates>(raw)) {}

cgss::DrawState::DrawState(const DrawState& other) : m_raw(std::make_unique<sf::RenderStates>(*other.m_raw)) {}

cgss::DrawState::DrawState(DrawState&& other) noexcept = default;

cgss::DrawState& cgss::DrawState::operator=(const DrawState& other) {
	*m_raw = *other.m_raw;
	return *this;
}

cgss::DrawState& cgss::DrawState::operator=(DrawState&& other) noexcept = default;

cgss::DrawState::~DrawState() = default;

sf::RenderStates& cgss::DrawState::raw() noexcept { return *m_raw; }
const sf::RenderStates& cgss::DrawState::raw() const noexcept { return *m_raw; }
