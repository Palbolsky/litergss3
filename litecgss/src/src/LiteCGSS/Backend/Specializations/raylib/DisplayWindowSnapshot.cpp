// Raylib stub — under CGSS_BACKEND=raylib, the Snapshot member of
// basic_DisplayWindow swaps to `StubDisplayWindowSnapshot<...>` (defined
// inline in Views/DisplayWindowSnapshot.h) via std::conditional_t. The
// non-template `cgss::DisplayWindowSnapshot` class methods are NOT used
// at all, so this file is intentionally empty — kept as a marker that the
// per-backend folder structure is symmetric.
//
// If a real raylib snapshot impl is ever needed, it would build on
// LoadImageFromScreen / GetScreenData2 — completely different from SFML's
// sf::Texture::update(window, x, y) approach.
