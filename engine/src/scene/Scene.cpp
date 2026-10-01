#include <gemedev/Scene.hpp>

#include "graphics/Renderer2D.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gd {
    namespace {
        std::atomic<std::uint64_t> nextSceneId{1};
    } // namespace

    /** Constructs a standalone scene with its own UI and animation services. */
    Scene::Scene(std::string name)
        : name_(std::move(name)),
          id_(nextSceneId.fetch_add(1)) {
    }

    /** Allocates a fresh slot or safely reuses a previously destroyed slot. */
    Entity Scene::createEntity(std::string name) {
        if (!name.empty() && valid(findEntity(name))) {
            throw std::invalid_argument("Entity name already exists: " + name);
        }

        std::uint32_t index;

        if (!free_.empty()) {
            index = free_.back();
            free_.pop_back();
        } else {
            index = static_cast<std::uint32_t>(slots_.size());
            slots_.emplace_back();
        }

        auto &slot = slots_[index];
        slot.alive = true;
        slot.name = std::move(name);

        return {index, slot.generation, id_};
    }

    /** Discards components and advances the slot generation. */
    void Scene::destroyEntity(Entity entity) {
        auto &slot = checked(entity);
        animations_.cancel(entity);

        const auto nextGeneration = slot.generation + 1;
        slot = Slot{};
        slot.generation = nextGeneration;

        free_.push_back(entity.index);
    }

    /** Checks a slot index, generation, and live flag without throwing. */
    bool Scene::valid(Entity entity) const noexcept {
        return entity.sceneId == id_ &&
               entity.index < slots_.size() &&
               slots_[entity.index].alive &&
               slots_[entity.index].generation == entity.generation;
    }

    /** Returns a mutable slot after rejecting invalid handles. */
    Scene::Slot &Scene::checked(Entity entity) {
        if (!valid(entity)) {
            throw std::out_of_range(
                "Entity handle is stale or belongs to another scene"
            );
        }

        return slots_[entity.index];
    }

    /** Returns a read-only slot after rejecting invalid handles. */
    const Scene::Slot &Scene::checked(Entity entity) const {
        if (!valid(entity)) {
            throw std::out_of_range(
                "Entity handle is stale or belongs to another scene"
            );
        }

        return slots_[entity.index];
    }

    /** Finds a live entity by its application-defined name. */
    Entity Scene::findEntity(const std::string &name) const noexcept {
        for (std::uint32_t i = 0; i < slots_.size(); ++i) {
            if (slots_[i].alive && slots_[i].name == name) {
                return {i, slots_[i].generation, id_};
            }
        }

        return {};
    }

    /** Returns an entity's persistent name. */
    const std::string &Scene::entityName(Entity entity) const {
        return checked(entity).name;
    }

    /** Replaces an entity's transform. */
    Transform &Scene::add(Entity entity, Transform value) {
        return checked(entity).transform.emplace(std::move(value));
    }

    /** Replaces an entity's sprite. */
    Sprite &Scene::add(Entity entity, Sprite value) {
        return checked(entity).sprite.emplace(std::move(value));
    }

    /** Replaces an entity's shape. */
    Shape &Scene::add(Entity entity, Shape value) {
        return checked(entity).shape.emplace(std::move(value));
    }

    /** Replaces an entity's text. */
    Text &Scene::add(Entity entity, Text value) {
        return checked(entity).text.emplace(std::move(value));
    }

    /** Replaces an entity's movement. */
    Motion &Scene::add(Entity entity, Motion value) {
        return checked(entity).motion.emplace(std::move(value));
    }

    /** Returns an optional mutable transform without throwing. */
    Transform *Scene::transform(Entity entity) noexcept {
        return valid(entity) && slots_[entity.index].transform
                   ? &*slots_[entity.index].transform
                   : nullptr;
    }

    /** Returns an optional read-only transform without throwing. */
    const Transform *Scene::transform(Entity entity) const noexcept {
        return valid(entity) && slots_[entity.index].transform
                   ? &*slots_[entity.index].transform
                   : nullptr;
    }

    /** Returns an optional sprite without throwing. */
    Sprite *Scene::sprite(Entity entity) noexcept {
        return valid(entity) && slots_[entity.index].sprite
                   ? &*slots_[entity.index].sprite
                   : nullptr;
    }

    /** Returns an optional shape without throwing. */
    Shape *Scene::shape(Entity entity) noexcept {
        return valid(entity) && slots_[entity.index].shape
                   ? &*slots_[entity.index].shape
                   : nullptr;
    }

    /** Returns optional text without throwing. */
    Text *Scene::text(Entity entity) noexcept {
        return valid(entity) && slots_[entity.index].text
                   ? &*slots_[entity.index].text
                   : nullptr;
    }

    /** Registers a click handler on an existing entity. */
    void Scene::onClick(Entity entity, std::function<void()> callback) {
        checked(entity).click = std::move(callback);
    }

    /** Creates a builder only for a live entity. */
    AnimationBuilder Scene::animate(Entity entity) {
        checked(entity);
        return AnimationBuilder(*this, entity);
    }

    /** Associates a persistent JSON-compatible value with a key. */
    void Scene::setValue(std::string key, SaveValue value) {
        values_[std::move(key)] = std::move(value);
    }

    /** Looks up a persistent value without modifying the scene. */
    const SaveValue *Scene::value(const std::string &key) const noexcept {
        const auto found = values_.find(key);
        return found == values_.end() ? nullptr : &found->second;
    }

    /** Integrates velocities at a fixed time step. */
    void Scene::updateFixed(float dt) {
        if (paused_) {
            return;
        }

        for (auto &slot: slots_) {
            if (slot.alive && slot.transform && slot.motion) {
                slot.transform->position =
                        slot.transform->position + slot.motion->velocity * dt;
            }
        }
    }

    /** Advances all current property tweens. */
    void Scene::update(float dt) {
        if (!paused_) {
            animations_.update(dt);
        }
    }

    /** Invokes user-defined gameplay logic before fixed-step movement. */
    void Scene::tick(const Input &input, float dt) {
        if (updateCallback_) {
            updateCallback_(*this, input, dt);
        }
    }

    /** Invalidates all live entity handles while keeping their slot generations. */
    void Scene::clearEntities() {
        animations_.clear();
        free_.clear();

        for (std::uint32_t i = 0; i < slots_.size(); ++i) {
            const auto generation =
                    slots_[i].generation + (slots_[i].alive ? 1U : 0U);

            slots_[i] = Slot{};
            slots_[i].generation = generation;
            free_.push_back(i);
        }
    }

    /** Resets all state that belongs to the scene. */
    void Scene::clear() {
        clearEntities();
        values_.clear();
        ui_.clear();
        overlay_ = {};
        updateCallback_ = {};
        camera_ = {};
        paused_ = false;
        backgroundTexture_ = {};
    }

    /** Routes screen-space UI clicks before testing world-space drawables. */
    void Scene::dispatchClick(
        Vec2 screenPoint,
        int viewportWidth,
        int viewportHeight
    ) {
        if (ui_.handleClick(screenPoint)) {
            return;
        }

        const Entity hit = hitTest(
            *this,
            screenPoint,
            viewportWidth,
            viewportHeight
        );

        if (valid(hit)) {
            auto callback = slots_[hit.index].click;

            if (callback) {
                callback();
            }
        }
    }

    /** Renders drawables in layer order, followed by overlay and UI. */
    void Scene::render(Renderer2D &renderer) {
        struct Draw {
            int layer;
            std::uint32_t index;
            int kind;
        };

        std::vector<Draw> drawables;

        for (std::uint32_t i = 0; i < slots_.size(); ++i) {
            const auto &slot = slots_[i];

            if (!slot.alive || !slot.transform) {
                continue;
            }

            if (slot.sprite) {
                drawables.push_back({slot.sprite->layer, i, 0});
            }

            if (slot.shape) {
                drawables.push_back({slot.shape->layer, i, 1});
            }

            if (slot.text) {
                drawables.push_back({slot.text->layer, i, 2});
            }
        }

        std::stable_sort(
            drawables.begin(),
            drawables.end(),
            [](const Draw &a, const Draw &b) {
                return a.layer < b.layer;
            }
        );

        for (const auto &drawable: drawables) {
            const auto &slot = slots_[drawable.index];
            Transform renderTransform = *slot.transform;

            // Relative is anchored to the screen, so the camera must not affect it.
            if (renderTransform.kind == TransformKind::Absolute) {
                renderTransform.position =
                        renderTransform.position - camera_;
            }

            if (drawable.kind == 0) {
                const Vec2 inputSize = slot.sprite->texture
                                           ? slot.sprite->texture->size()
                                           : Vec2{64.0f, 64.0f};

                renderer.draw(
                    ShapeKind::Rectangle,
                    renderTransform,
                    slot.sprite->size(inputSize),
                    slot.sprite->tint,
                    slot.sprite->texture
                );
            } else if (drawable.kind == 1) {
                renderer.draw(
                    slot.shape->kind,
                    renderTransform,
                    slot.shape->size,
                    slot.shape->tint,
                    slot.shape->texture
                );
            } else if (drawable.kind == 2) {
                renderer.drawText(
                    renderTransform,
                    slot.text->value,
                    slot.text->font,
                    slot.text->tint
                );
            }
        }

        Canvas canvas(renderer);

        if (overlay_) {
            overlay_(canvas);
        }

        ui_.draw(canvas);
    }
} // namespace gd
