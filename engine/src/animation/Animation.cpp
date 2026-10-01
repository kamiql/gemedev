#include <gemedev/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace gd {
    namespace {
        float eased(float t, Ease ease) {
            t = std::clamp(t, 0.0f, 1.0f);

            switch (ease) {
                case Ease::Linear:
                    return t;

                case Ease::InQuad:
                    return t * t;

                case Ease::OutCubic:
                    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

                case Ease::InOutQuad:
                    return t < 0.5f
                               ? 2.0f * t * t
                               : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
            }

            return t;
        }

        void validateDuration(float seconds) {
            if (!std::isfinite(seconds) || seconds <= 0.0f) {
                throw std::invalid_argument(
                    "Animation duration must be finite and positive"
                );
            }
        }
    } // namespace

    AnimationBuilder::AnimationBuilder(Scene &scene, Entity entity)
        : scene_(scene),
          entity_(entity),
          sequenceId_(++scene.animations_.nextSequenceId_) {
    }

    /** Schedules a position tween in the current phase. */
    AnimationBuilder &AnimationBuilder::positionTo(
        Vec2 target, float seconds, Ease ease
    ) {
        validateDuration(seconds);

        if (!scene_.transform(entity_)) {
            throw std::logic_error(
                "Position tween requires a Transform component"
            );
        }

        Scene *scene = &scene_;
        const Entity entity = entity_;

        scene_.animations_.queue(
            sequenceId_,
            phase_,
            {
                entity,
                AnimationSystem::Property::Position,
                0.0f,
                seconds,
                ease,
                [scene, entity, target]() -> std::function<void(float)> {
                    const auto *transform = scene->transform(entity);
                    if (!transform) return {};

                    const Vec2 from = transform->position;

                    return [scene, entity, from, target](float t) {
                        if (auto *current = scene->transform(entity)) {
                            current->position =
                                    from + (target - from) * t;
                        }
                    };
                },
                {}
            }
        );

        hasTweenInPhase_ = true;
        return *this;
    }

    /** Schedules a scale tween in the current phase. */
    AnimationBuilder &AnimationBuilder::scaleTo(
        Vec2 target, float seconds, Ease ease
    ) {
        validateDuration(seconds);

        if (!scene_.transform(entity_)) {
            throw std::logic_error(
                "Scale tween requires a Transform component"
            );
        }

        Scene *scene = &scene_;
        const Entity entity = entity_;

        scene_.animations_.queue(
            sequenceId_,
            phase_,
            {
                entity,
                AnimationSystem::Property::Scale,
                0.0f,
                seconds,
                ease,
                [scene, entity, target]() -> std::function<void(float)> {
                    const auto *transform = scene->transform(entity);
                    if (!transform) return {};

                    const Vec2 from = transform->scale;

                    return [scene, entity, from, target](float t) {
                        if (auto *current = scene->transform(entity)) {
                            current->scale =
                                    from + (target - from) * t;
                        }
                    };
                },
                {}
            }
        );

        hasTweenInPhase_ = true;
        return *this;
    }

    /** Schedules an opacity tween in the current phase. */
    AnimationBuilder &AnimationBuilder::opacityTo(
        float target, float seconds, Ease ease
    ) {
        validateDuration(seconds);

        if (target < 0.0f || target > 1.0f) {
            throw std::invalid_argument(
                "Opacity must be between 0 and 1"
            );
        }

        if (!scene_.sprite(entity_) &&
            !scene_.shape(entity_) &&
            !scene_.text(entity_)) {
            throw std::logic_error(
                "Opacity tween requires a drawable"
            );
        }

        Scene *scene = &scene_;
        const Entity entity = entity_;

        scene_.animations_.queue(
            sequenceId_,
            phase_,
            {
                entity,
                AnimationSystem::Property::Opacity,
                0.0f,
                seconds,
                ease,
                [scene, entity, target]() -> std::function<void(float)> {
                    const auto *sprite = scene->sprite(entity);
                    const auto *shape = scene->shape(entity);
                    const auto *text = scene->text(entity);

                    if (!sprite && !shape && !text) return {};

                    const float spriteFrom =
                            sprite ? sprite->tint.a : 1.0f;
                    const float shapeFrom =
                            shape ? shape->tint.a : 1.0f;
                    const float textFrom =
                            text ? text->tint.a : 1.0f;

                    return [
                                scene, entity, target,
                                spriteFrom, shapeFrom, textFrom
                            ](float t) {
                        if (auto *current = scene->sprite(entity)) {
                            current->tint.a =
                                    spriteFrom + (target - spriteFrom) * t;
                        }

                        if (auto *current = scene->shape(entity)) {
                            current->tint.a =
                                    shapeFrom + (target - shapeFrom) * t;
                        }

                        if (auto *current = scene->text(entity)) {
                            current->tint.a =
                                    textFrom + (target - textFrom) * t;
                        }
                    };
                },
                {}
            }
        );

        hasTweenInPhase_ = true;
        return *this;
    }

    /** Makes subsequent tweens wait for the current phase. */
    AnimationBuilder &AnimationBuilder::then() {
        if (!hasTweenInPhase_) {
            throw std::logic_error(
                "then() requires an animation in the preceding phase"
            );
        }

        ++phase_;
        hasTweenInPhase_ = false;
        return *this;
    }

    /**
     * Adds a tween to a sequence.
     * A new sequence supersedes an older sequence if both animate
     * the same property of the same entity.
     */
    void AnimationSystem::queue(
        std::uint64_t sequenceId,
        std::size_t phaseIndex,
        Tween tween
    ) {
        std::erase_if(sequences_, [&](const Sequence &sequence) {
            if (sequence.id == sequenceId) return false;

            for (const auto &phase: sequence.phases) {
                for (const auto &existing: phase.tweens) {
                    if (existing.entity == tween.entity &&
                        existing.property == tween.property) {
                        return true;
                    }
                }
            }

            return false;
        });

        auto sequenceIt = std::find_if(
            sequences_.begin(),
            sequences_.end(),
            [sequenceId](const Sequence &sequence) {
                return sequence.id == sequenceId;
            }
        );

        if (sequenceIt == sequences_.end()) {
            sequences_.push_back(Sequence{sequenceId});
            sequenceIt = std::prev(sequences_.end());
        }

        auto &sequence = *sequenceIt;

        if (phaseIndex > sequence.phases.size()) {
            throw std::logic_error("Animation phase is not contiguous");
        }

        if (phaseIndex == sequence.phases.size()) {
            sequence.phases.emplace_back();
        }

        auto &phase = sequence.phases[phaseIndex];

        // Zweimal dieselbe Property innerhalb derselben Phase:
        // Der zuletzt registrierte Tween gewinnt.
        std::erase_if(phase.tweens, [&](const Tween &existing) {
            return existing.entity == tween.entity &&
                   existing.property == tween.property;
        });

        // Phase 0 läuft sofort. Spätere Phasen werden erst beim
        // Phasenwechsel initialisiert.
        if (phaseIndex == sequence.currentPhase && phase.initialized) {
            tween.apply = tween.makeApply();
        }

        phase.tweens.push_back(std::move(tween));

        if (phaseIndex == sequence.currentPhase && !phase.initialized) {
            for (auto &current: phase.tweens) {
                current.apply = current.makeApply();
            }
            phase.initialized = true;
        }
    }

    /** Registers a callback to run after all phases in this sequence complete. */
    AnimationBuilder &AnimationBuilder::onComplete(
        std::function<void()> callback
    ) {
        auto sequenceIt = std::find_if(
            scene_.animations_.sequences_.begin(),
            scene_.animations_.sequences_.end(),
            [this](const auto &sequence) {
                return sequence.id == sequenceId_;
            }
        );

        if (sequenceIt == scene_.animations_.sequences_.end()) {
            throw std::logic_error(
                "onComplete() requires an animation in this sequence"
            );
        }

        sequenceIt->onComplete = std::move(callback);
        return *this;
    }

    /** Advances each sequence and carries leftover frame time into the next phase. */
    void AnimationSystem::update(float dt) {
        if (!std::isfinite(dt) || dt < 0.0f) {
            return;
        }

        for (auto &sequence: sequences_) {
            float remaining = dt;

            while (sequence.currentPhase < sequence.phases.size()) {
                auto &phase = sequence.phases[sequence.currentPhase];

                if (!phase.initialized) {
                    for (auto &tween: phase.tweens) {
                        tween.apply = tween.makeApply();
                    }

                    phase.initialized = true;
                }

                float timeUntilPhaseEnds = 0.0f;

                for (const auto &tween: phase.tweens) {
                    timeUntilPhaseEnds = std::max(
                        timeUntilPhaseEnds,
                        tween.duration - tween.elapsed
                    );
                }

                const float step = std::min(
                    remaining,
                    timeUntilPhaseEnds
                );

                if (step > 0.0f) {
                    for (auto &tween: phase.tweens) {
                        tween.elapsed = std::min(
                            tween.duration,
                            tween.elapsed + step
                        );

                        if (tween.apply) {
                            tween.apply(eased(
                                tween.elapsed / tween.duration,
                                tween.ease
                            ));
                        }
                    }
                }

                remaining -= step;

                if (step < timeUntilPhaseEnds) {
                    break;
                }

                // Die nächste Phase startet erst, nachdem die aktuelle
                // vollständig abgeschlossen wurde.
                ++sequence.currentPhase;
            }
        }

        // Erst abgeschlossene Sequenzen und deren Callbacks einsammeln.
        std::vector<std::function<void()> > completedCallbacks;

        for (auto &sequence: sequences_) {
            if (sequence.currentPhase >= sequence.phases.size() &&
                sequence.onComplete) {
                completedCallbacks.push_back(std::move(sequence.onComplete));
            }
        }

        // Fertige Sequenzen entfernen, bevor Callbacks ausgeführt werden.
        std::erase_if(sequences_, [](const Sequence &sequence) {
            return sequence.currentPhase >= sequence.phases.size();
        });

        // Callbacks dürfen hier neue Animationen registrieren.
        for (auto &callback: completedCallbacks) {
            callback();
        }
    }

    /** Cancels all phases belonging to this entity. */
    void AnimationSystem::cancel(Entity entity) {
        std::erase_if(sequences_, [entity](const Sequence &sequence) {
            for (const auto &phase: sequence.phases) {
                for (const auto &tween: phase.tweens) {
                    if (tween.entity == entity) return true;
                }
            }

            return false;
        });
    }

    /** Discards all sequences. */
    void AnimationSystem::clear() {
        sequences_.clear();
    }
} // namespace gd
