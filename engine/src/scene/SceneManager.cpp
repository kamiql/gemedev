#include <gemedev/SceneManager.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace gd {
    /** Creates a scene while preserving references to existing scenes. */
    Scene &SceneManager::create(const std::string &name) {
        if (name.empty()) {
            throw std::invalid_argument("Scene name must not be empty");
        }

        if (scenes_.contains(name)) {
            throw std::invalid_argument("Scene already exists: " + name);
        }

        auto scene = std::make_unique<Scene>(name);
        Scene *result = scene.get();

        scenes_.emplace(name, std::move(scene));

        if (!active_) {
            active_ = result;
        }

        return *result;
    }

    /** Activates an existing scene immediately and cancels any transition. */
    void SceneManager::activate(const std::string &name) {
        Scene *selected = find(name);

        if (!selected) {
            throw std::out_of_range("Unknown scene: " + name);
        }

        active_ = selected;
        transitionSource_ = nullptr;
        transitionElapsed_ = 0.0f;
        transitionOnComplete_ = {};
    }

    /** Starts a transition and stores its completion callback. */
    void SceneManager::transitionTo(
        const std::string &name,
        const Transition &transition,
        std::function<void()> onComplete
    ) {
        Scene *selected = find(name);

        if (!selected) {
            throw std::out_of_range("Unknown scene: " + name);
        }

        // Replacing a transition cancels its previous completion callback.
        transitionOnComplete_ = {};

        // No visual transition is needed in these cases.
        if (
            !active_ ||
            selected == active_ ||
            !std::isfinite(transition.duration) ||
            transition.duration <= 0.0f
        ) {
            active_ = selected;
            transitionSource_ = nullptr;
            transitionElapsed_ = 0.0f;

            if (onComplete) {
                onComplete();
            }

            return;
        }

        transitionSource_ = active_;
        active_ = selected;
        transition_ = transition;
        transitionElapsed_ = 0.0f;
        transitionOnComplete_ = std::move(onComplete);
    }

    /** Returns the active scene or throws when there are no scenes. */
    Scene &SceneManager::active() {
        if (!active_) {
            throw std::logic_error(
                "Create a scene before starting the application"
            );
        }

        return *active_;
    }

    /** Returns the active scene or throws when there are no scenes. */
    const Scene &SceneManager::active() const {
        if (!active_) {
            throw std::logic_error(
                "Create a scene before starting the application"
            );
        }

        return *active_;
    }

    /** Looks up a named scene without changing active state. */
    Scene *SceneManager::find(const std::string &name) const noexcept {
        const auto it = scenes_.find(name);

        if (it == scenes_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    /** Returns the current transition progress clamped to [0, 1]. */
    float SceneManager::transitionProgress() const noexcept {
        if (!transitionSource_) {
            return 1.0f;
        }

        if (transition_.duration <= 0.0f) {
            return 1.0f;
        }

        return std::clamp(
            transitionElapsed_ / transition_.duration,
            0.0f,
            1.0f
        );
    }

    /** Advances the transition and invokes its callback when complete. */
    void SceneManager::advanceTransition(float dt) {
        if (!transitionSource_) {
            return;
        }

        if (std::isfinite(dt) && dt > 0.0f) {
            transitionElapsed_ += dt;
        }

        if (transitionElapsed_ >= transition_.duration) {
            transitionSource_ = nullptr;
            transitionElapsed_ = 0.0f;

            const auto onComplete = std::move(transitionOnComplete_);
            transitionOnComplete_ = {};

            if (onComplete) {
                onComplete();
            }
        }
    }
} // namespace gd
