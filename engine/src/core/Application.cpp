#include <gemedev/Application.hpp>
#include "graphics/Renderer2D.hpp"
#include "platform/GlfwWindow.hpp"
#include <algorithm>
#include <chrono>

namespace gd {
    /** Owns all services in an order that keeps GL resources alive before the window. */
    struct Application::Impl {
        GlfwWindow window;
        AssetCache assets;
        Renderer2D renderer;
        SceneManager scenes;
        AudioSystem audio;
        SaveService saves;
        Input input;
        bool quitting = false;
        double runtimeSeconds = 0.0;
        float deltaTime = 0.0f;

        /** Initializes window-dependent services after making the context current. */
        explicit Impl(const AppConfig &config)
            : window(config.title, config.width, config.height, config.vsync), saves(assets) {
        }
    };

    /** Creates a new GLFW/GLEW window and service graph. */
    Application::Application(AppConfig config) : impl_(std::make_unique<Impl>(config)) {
    }

    /** Destroys scene-held assets before the OpenGL context is closed. */
    Application::~Application() = default;

    /** Runs deterministic movement updates and variable-step animations per frame. */
    void Application::run() const {
        auto &state = *impl_;
        state.scenes.active();

        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();
        auto last = started;

        state.runtimeSeconds = 0.0;
        state.deltaTime = 0.0f;

        double accumulated = 0.0;
        constexpr double step = 1.0 / 60.0;

        while (!state.quitting && !state.window.shouldClose()) {
            const auto now = Clock::now();

            state.runtimeSeconds =
                    std::chrono::duration<double>(now - started).count();

            const float dt = std::min(
                0.1f,
                std::chrono::duration<float>(now - last).count()
            );
            state.deltaTime = dt;
            last = now;

            state.window.poll(state.input);

            if (state.input.mousePressed(MouseButton::Left)) {
                auto &scene = state.scenes.active();

                scene.dispatchClick(
                    state.input.mousePosition(),
                    state.window.windowWidth(),
                    state.window.windowHeight()
                );
            }

            auto &scene = state.scenes.active();
            scene.tick(state.input, dt);

            accumulated += dt;
            int iterations = 0;
            while (accumulated >= step && iterations < 6) {
                scene.updateFixed(static_cast<float>(step));
                accumulated -= step;
                ++iterations;
            }
            if (iterations == 6) accumulated = 0;

            scene.update(dt);
            state.renderer.begin(
                state.window.framebufferWidth(),
                state.window.framebufferHeight(),
                state.window.windowWidth(),
                state.window.windowHeight(),
                scene.background()
            );
            state.renderer.drawBackground(scene.backgroundTexture());
            scene.render(state.renderer);
            state.window.swap();
        }

        state.runtimeSeconds =
                std::chrono::duration<double>(Clock::now() - started).count();
    }

    /** Marks the application for exit at the end of its current frame. */
    void Application::quit() noexcept { impl_->quitting = true; }

    int Application::width() const noexcept {
        return impl_->window.windowWidth();
    }

    int Application::height() const noexcept {
        return impl_->window.windowHeight();
    }

    /** Returns the context-bound resource cache. */
    AssetCache &Application::assets() noexcept { return impl_->assets; }
    /** Returns the audio service. */
    AudioSystem &Application::audio() noexcept { return impl_->audio; }
    /** Returns named scenes. */
    SceneManager &Application::scenes() noexcept { return impl_->scenes; }
    /** Returns the current input snapshot. */
    const Input &Application::input() const noexcept { return impl_->input; }
    /** Returns the scene persistence service. */
    SaveService &Application::saves() noexcept { return impl_->saves; }

    double Application::runtimeSeconds() const noexcept {
        return impl_->runtimeSeconds;
    }

    float Application::deltaTime() const noexcept {
        return impl_->deltaTime;
    }
}
