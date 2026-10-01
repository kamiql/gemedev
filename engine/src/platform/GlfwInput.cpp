#include <gemedev/Input.hpp>
#include <cstddef>

namespace gd {
/** Returns whether a key is currently held. */
bool Input::keyDown(Key key) const noexcept {
    const auto index = static_cast<int>(key);
    return index >= 0 && index < static_cast<int>(keys_.size()) && keys_[static_cast<std::size_t>(index)];
}
/** Detects a key's unpressed-to-pressed transition. */
bool Input::keyPressed(Key key) const noexcept {
    const auto index = static_cast<int>(key);
    return index >= 0 && index < static_cast<int>(keys_.size()) && keys_[static_cast<std::size_t>(index)] && !previousKeys_[static_cast<std::size_t>(index)];
}
/** Returns whether a mouse button is currently held. */
bool Input::mouseDown(MouseButton button) const noexcept {
    const auto index = static_cast<int>(button);
    return index >= 0 && index < static_cast<int>(buttons_.size()) && buttons_[static_cast<std::size_t>(index)];
}
/** Detects a mouse button's unpressed-to-pressed transition. */
bool Input::mousePressed(MouseButton button) const noexcept {
    const auto index = static_cast<int>(button);
    return index >= 0 && index < static_cast<int>(buttons_.size()) && buttons_[static_cast<std::size_t>(index)] && !previousButtons_[static_cast<std::size_t>(index)];
}
}
