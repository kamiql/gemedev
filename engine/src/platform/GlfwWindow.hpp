#pragma once
#include <gemedev/Input.hpp>
#include <string>

struct GLFWwindow;
namespace gd {
/** Wraps GLFW initialization, OpenGL context setup, and window event polling. */
class GlfwWindow {
public:
    /** Creates a resizable GL 3.3 core window (GL 4.1 on macOS). */
    GlfwWindow(const std::string& title, int width, int height, bool vsync);
    /** Destroys the GL context and releases GLFW. */
    ~GlfwWindow();
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    /** Polls GLFW events and updates the input snapshot. */
    void poll(Input& input);
    /** Returns whether the operating system requested closure. */
    bool shouldClose() const;
    /** Swaps the front and back buffers. */
    void swap();
    /** Returns the drawable framebuffer width in physical pixels. */
    int framebufferWidth() const;
    /** Returns the drawable framebuffer height in physical pixels. */
    int framebufferHeight() const;
    /** Returns the logical window width used for pointer coordinates. */
    int windowWidth() const;
    /** Returns the logical window height used for pointer coordinates. */
    int windowHeight() const;
private:
    GLFWwindow* window_ = nullptr;
};
}
