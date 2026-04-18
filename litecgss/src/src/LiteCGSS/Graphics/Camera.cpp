#include "Camera.h"
#include <SFML/Graphics/View.hpp>

cgss::Camera::Camera() : m_view(std::make_unique<sf::View>()) {}

cgss::Camera::Camera(const sf::View& view) : m_view(std::make_unique<sf::View>(view)) {}

cgss::Camera::Camera(const Camera& other) : m_view(std::make_unique<sf::View>(*other.m_view)) {}

cgss::Camera::Camera(Camera&& other) noexcept = default;

cgss::Camera& cgss::Camera::operator=(const Camera& other) {
	*m_view = *other.m_view;
	return *this;
}

cgss::Camera& cgss::Camera::operator=(Camera&& other) noexcept = default;

cgss::Camera::~Camera() = default;

sf::View& cgss::Camera::raw() noexcept { return *m_view; }

const sf::View& cgss::Camera::raw() const noexcept { return *m_view; }
