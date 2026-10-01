#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "platform/GlfwWindow.hpp"
#include <stdexcept>
#include <string>

namespace gd {
/** Creates the GL context and initializes its function loader. */
GlfwWindow::GlfwWindow(const std::string& title, int width, int height, bool vsync) {
    if (width <= 0 || height <= 0) throw std::invalid_argument("Window dimensions must be positive");
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Could not create the GLFW window");
    }
    glfwMakeContextCurrent(window_);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
        throw std::runtime_error("glewInit failed");
    }
    glGetError();
    glfwSwapInterval(vsync ? 1 : 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int, int action, int) {
        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(w));
        if (input && key >= 0 && key < 512 && action != GLFW_REPEAT)
            input->keys_[static_cast<std::size_t>(key)] = action == GLFW_PRESS;
    });
    glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int) {
        auto* input = static_cast<Input*>(glfwGetWindowUserPointer(w));
        if (input && button >= 0 && button < 8)
            input->buttons_[static_cast<std::size_t>(button)] = action == GLFW_PRESS;
    });
}
/** Tears down GLFW only after all context-dependent members have been destroyed. */
GlfwWindow::~GlfwWindow() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}
/** Processes events after preserving last frame's key and mouse state. */
void GlfwWindow::poll(Input& input) {
    input.previousKeys_ = input.keys_;
    input.previousButtons_ = input.buttons_;
    glfwSetWindowUserPointer(window_, &input);
    glfwPollEvents();
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    input.mouse_ = {static_cast<float>(x), static_cast<float>(y)};
}
/** Reports GLFW's close flag. */
bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(window_) != 0; }
/** Presents the current framebuffer. */
void GlfwWindow::swap() { glfwSwapBuffers(window_); }
/** Returns the physical framebuffer width. */
int GlfwWindow::framebufferWidth() const { int w, h; glfwGetFramebufferSize(window_, &w, &h); return w; }
/** Returns the physical framebuffer height. */
int GlfwWindow::framebufferHeight() const { int w, h; glfwGetFramebufferSize(window_, &w, &h); return h; }
/** Returns the logical window width. */
int GlfwWindow::windowWidth() const { int w, h; glfwGetWindowSize(window_, &w, &h); return w; }
/** Returns the logical window height. */
int GlfwWindow::windowHeight() const { int w, h; glfwGetWindowSize(window_, &w, &h); return h; }
}
