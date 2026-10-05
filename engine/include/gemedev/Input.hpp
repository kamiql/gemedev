#pragma once
#include <gemedev/Types.hpp>
#include <array>

namespace gd {
/** Frequently used keys; values follow the GLFW keyboard convention. */
enum class Key : int { Space = 32, A = 65, D = 68, S = 83, W = 87, Shift = 344, Escape = 256, Left = 263, Right = 262, Up = 265, Down = 264 };
/** Mouse buttons exposed without depending on GLFW headers. */
enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };
/** A snapshot of key, mouse, and pointer state for the current frame. */
class Input {
public:
    /** Returns whether a key is held this frame. */
    bool keyDown(Key key) const noexcept;
    /** Returns whether a key became pressed during this frame. */
    bool keyPressed(Key key) const noexcept;
    /** Returns whether a mouse button is held this frame. */
    bool mouseDown(MouseButton button) const noexcept;
    /** Returns whether a mouse button became pressed during this frame. */
    bool mousePressed(MouseButton button) const noexcept;
    /** Returns the pointer location in logical window coordinates. */
    Vec2 mousePosition() const noexcept { return mouse_; }
private:
    friend class GlfwWindow;
    std::array<bool, 512> keys_{};
    std::array<bool, 512> previousKeys_{};
    std::array<bool, 8> buttons_{};
    std::array<bool, 8> previousButtons_{};
    Vec2 mouse_{};
};
}
