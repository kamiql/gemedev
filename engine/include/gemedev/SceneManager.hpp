#pragma once

#include <gemedev/Scene.hpp>
#include <gemedev/Types.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace gd {
    /** Visual style used when moving from one scene to another. */
    enum class TransitionType {
        CrossFade,
        SlideLeft,
        SlideRight,
        FadeThroughColor
    };

    /** Options for a scene transition. Durations are measured in seconds. */
    struct Transition {
        TransitionType type = TransitionType::CrossFade;
        float duration = 0.35f;
        Color color{0.0f, 0.0f, 0.0f, 1.0f};

        static Transition crossFade(float duration = 0.35f) noexcept {
            return {
                TransitionType::CrossFade,
                duration,
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
        }

        static Transition slideLeft(float duration = 0.4f) noexcept {
            return {
                TransitionType::SlideLeft,
                duration,
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
        }

        static Transition slideRight(float duration = 0.4f) noexcept {
            return {
                TransitionType::SlideRight,
                duration,
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
        }

        static Transition fadeThrough(
            Color color,
            float duration = 0.5f
        ) noexcept {
            return {
                TransitionType::FadeThroughColor,
                duration,
                color
            };
        }
    };

    /** Owns named scenes, tracks the active scene, and animates scene changes. */
    class SceneManager {
    public:
        /** Creates a named scene and makes it active when none was active. */
        Scene &create(const std::string &name);

        /** Activates an existing scene immediately and cancels any transition. */
        void activate(const std::string &name);

        /**
         * Starts a visual transition to an existing scene.
         * Calls onComplete once the transition has finished.
         */
        void transitionTo(
            const std::string &name,
            const Transition &transition = Transition::crossFade(),
            std::function<void()> onComplete = {}
        );

        /** Returns the active destination scene during a transition. */
        Scene &active();

        /** Returns the active destination scene during a transition. */
        const Scene &active() const;

        /** Returns a scene by name or null if it does not exist. */
        Scene *find(const std::string &name) const noexcept;

        /** Returns true while a source scene is still being rendered. */
        bool transitioning() const noexcept {
            return transitionSource_ != nullptr;
        }

        /** Returns the outgoing scene during a transition, otherwise null. */
        Scene *transitionSource() const noexcept {
            return transitionSource_;
        }

        /** Returns normalized transition progress, or 1 when idle. */
        float transitionProgress() const noexcept;

        /** Returns the current transition settings. */
        const Transition &currentTransition() const noexcept {
            return transition_;
        }

    private:
        friend class Application;

        void advanceTransition(float dt);

        std::unordered_map<std::string, std::unique_ptr<Scene> > scenes_;

        Scene *active_ = nullptr;
        Scene *transitionSource_ = nullptr;

        Transition transition_{};
        float transitionElapsed_ = 0.0f;

        std::function<void()> transitionOnComplete_;
    };
} // namespace gd
