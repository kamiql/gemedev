#pragma once
#include <gemedev/Assets.hpp>
#include <gemedev/Audio.hpp>
#include <gemedev/Input.hpp>
#include <gemedev/Save.hpp>
#include <gemedev/SceneManager.hpp>
#include <memory>
#include <string>

namespace gd {
    /** Window and rendering settings supplied at application startup. */
    struct AppConfig {
        std::string title = "Gemedev";
        int width = 1280;
        int height = 720;
        bool vsync = true;
    };

    /** Owns the GLFW/GLEW context, main loop, and all engine services. */
    class Application {
    public:
        /** Initializes the window, GL renderer, audio, and engine services. */
        explicit Application(AppConfig config = {});

        /** Stops services and releases GPU resources before the window is destroyed. */
        ~Application();

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

        /** Runs the frame loop until a close request or explicit quit. */
        void run() const;

        /** Requests that the frame loop exit after the current frame. */
        void quit() noexcept;

        /** Returns the asset loader and cache. */
        AssetCache &assets() noexcept;

        /** Returns the audio playback service. */
        AudioSystem &audio() noexcept;

        /** Returns the named-scene manager. */
        SceneManager &scenes() noexcept;

        /** Returns the current input snapshot. */
        const Input &input() const noexcept;

        /** Returns the JSON scene-save service. */
        SaveService &saves() noexcept;

        /** Seconds elapsed since the current run() started. */
        double runtimeSeconds() const noexcept;

        /** Frame time used for the most recent update, capped at 0.1 seconds. */
        float deltaTime() const noexcept;

        /** Current window width in logical screen coordinates. */
        int width() const noexcept;

        /** Current window height in logical screen coordinates. */
        int height() const noexcept;

        Vec2 middle() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
