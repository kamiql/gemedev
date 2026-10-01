#pragma once

#include <gemedev/Components.hpp>

#include <cstdint>
#include <functional>
#include <vector>

namespace gd {

class Scene;

/** Easing curves used by the built-in tweens. */
enum class Ease { Linear, InQuad, OutCubic, InOutQuad };

/**
 * Registers animations for one entity.
 * Calls within one phase run in parallel; then() starts a new phase.
 */
class AnimationBuilder {
public:
    AnimationBuilder(const AnimationBuilder&) = delete;
    AnimationBuilder& operator=(const AnimationBuilder&) = delete;

    AnimationBuilder& positionTo(
        Vec2 target, float seconds, Ease ease = Ease::Linear
    );

    AnimationBuilder& scaleTo(
        Vec2 target, float seconds, Ease ease = Ease::Linear
    );

    AnimationBuilder& opacityTo(
        float target, float seconds, Ease ease = Ease::Linear
    );

    /** Starts a phase after all animations of the preceding phase finish. */
    AnimationBuilder& then();

    AnimationBuilder& onComplete(std::function<void()> callback);

private:
    friend class Scene;

    AnimationBuilder(Scene& scene, Entity entity);

    Scene& scene_;
    Entity entity_;
    std::uint64_t sequenceId_ = 0;
    std::size_t phase_ = 0;
    bool hasTweenInPhase_ = false;
};

/** Updates animation sequences with parallel phases. */
class AnimationSystem {
public:
    void update(float dt);
    void cancel(Entity entity);
    void clear();

private:
    friend class AnimationBuilder;

    enum class Property { Position, Scale, Opacity };

    struct Tween {
        Entity entity{};
        Property property{};
        float elapsed = 0.0f;
        float duration = 0.0f;
        Ease ease = Ease::Linear;

        std::function<std::function<void(float)>()> makeApply;
        std::function<void(float)> apply;
    };

    struct Phase {
        std::vector<Tween> tweens;
        bool initialized = false;
    };

    struct Sequence {
        std::uint64_t id = 0;
        std::vector<Phase> phases;
        std::size_t currentPhase = 0;
        std::function<void()> onComplete;
    };

    void queue(
        std::uint64_t sequenceId,
        std::size_t phaseIndex,
        Tween tween
    );

    std::uint64_t nextSequenceId_ = 0;
    std::vector<Sequence> sequences_;
};

} // namespace gd