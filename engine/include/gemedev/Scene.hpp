#pragma once

#include <gemedev/Animation.hpp>
#include <gemedev/Canvas.hpp>
#include <gemedev/Components.hpp>
#include <gemedev/Input.hpp>
#include <gemedev/UI.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gd {
    class Renderer2D;
    class SaveService;
    class Application;

    /** A collection of entities, components, UI, animations, and persistent values. */
    class Scene {
    public:
        /** Creates an initially empty named scene without requiring a GL context. */
        explicit Scene(std::string name);

        Scene(const Scene &) = delete;

        Scene &operator=(const Scene &) = delete;

        Scene(Scene &&) = delete;

        Scene &operator=(Scene &&) = delete;

        void setBackground(Color color) noexcept {
            background_ = color;
            backgroundTexture_ = {};
        }

        void setBackground(TextureHandle texture) {
            backgroundTexture_ = std::move(texture);
        }

        [[nodiscard]] const TextureHandle &backgroundTexture() const noexcept {
            return backgroundTexture_;
        }

        /** Returns the scene identifier. */
        const std::string &name() const noexcept {
            return name_;
        }

        /** Allocates an entity and an optional persistent name. */
        Entity createEntity(std::string name = {});

        /** Destroys a live entity, its components, callback, and active tweens. */
        void destroyEntity(Entity entity);

        /** Returns whether an entity belongs to the current generation of this scene. */
        bool valid(Entity entity) const noexcept;

        /** Finds a live entity by persistent name, or returns an invalid handle. */
        Entity findEntity(const std::string &name) const noexcept;

        /** Returns the stable application-defined name assigned to an entity. */
        const std::string &entityName(Entity entity) const;

        /** Adds or replaces a transform and returns the stored component. */
        Transform &add(Entity entity, Transform value);

        /** Adds or replaces a sprite and returns the stored component. */
        Sprite &add(Entity entity, Sprite value);

        /** Adds or replaces a primitive shape and returns the stored component. */
        Shape &add(Entity entity, Shape value);

        /** Adds or replaces a text drawable and returns the stored component. */
        Text &add(Entity entity, Text value);

        /** Adds or replaces linear movement and returns the stored component. */
        Motion &add(Entity entity, Motion value);

        /** Returns a transform pointer, or null if it is not present. */
        Transform *transform(Entity entity) noexcept;

        /** Returns a read-only transform pointer, or null if it is not present. */
        const Transform *transform(Entity entity) const noexcept;

        /** Returns a sprite pointer, or null if it is not present. */
        Sprite *sprite(Entity entity) noexcept;

        /** Returns a shape pointer, or null if it is not present. */
        Shape *shape(Entity entity) noexcept;

        /** Returns a text pointer, or null if it is not present. */
        Text *text(Entity entity) noexcept;

        /** Sets or replaces the callback invoked when the drawable is clicked. */
        void onClick(Entity entity, std::function<void()> callback);

        /** Returns a fluent builder that registers independent property tweens. */
        AnimationBuilder animate(Entity entity);

        /** Returns the scene-local retained UI tree. */
        UIContext &ui() noexcept {
            return ui_;
        }

        /** Returns the scene-local retained UI tree. */
        const UIContext &ui() const noexcept {
            return ui_;
        }

        /** Stores a JSON-compatible scalar that is included in scene saves. */
        void setValue(std::string key, SaveValue value);

        /** Returns a persistent scalar or null when the key is missing. */
        const SaveValue *value(const std::string &key) const noexcept;

        /** Returns the scene clear color. */
        Color background() const noexcept {
            return background_;
        }

        /** Sets the world-space point mapped to the upper-left of the window. */
        void setCamera(Vec2 position) noexcept {
            camera_ = position;
        }

        /** Returns the world-space camera origin. */
        Vec2 camera() const noexcept {
            return camera_;
        }

        /** Pauses movement and animation while retaining rendering and UI input. */
        void setPaused(bool paused) noexcept {
            paused_ = paused;
        }

        /** Returns whether movement and animation are paused. */
        bool paused() const noexcept {
            return paused_;
        }

        /** Sets an optional immediate-mode drawing callback invoked after scene entities. */
        void setOverlay(std::function<void(Canvas &)> callback) {
            overlay_ = std::move(callback);
        }

        /** Sets gameplay logic called once per frame after input dispatch. */
        void setUpdateCallback(
            std::function<void(Scene &, const Input &, float)> callback
        ) {
            updateCallback_ = std::move(callback);
        }

        /** Advances fixed-step movement by a time interval in seconds. */
        void updateFixed(float dt);

        /** Advances property animations by a frame interval in seconds. */
        void update(float dt);

        /** Removes entities, UI, animations, and persistent values. */
        void clear();

    private:
        TextureHandle backgroundTexture_;

        friend class SaveService;
        friend class AnimationBuilder;
        friend class Application;

        friend Entity hitTest(
            const Scene &scene,
            Vec2 screenPoint,
            int viewportWidth,
            int viewportHeight
        );

        struct Slot {
            std::uint32_t generation = 1;
            bool alive = false;
            std::string name;
            std::optional<Transform> transform;
            std::optional<Sprite> sprite;
            std::optional<Shape> shape;
            std::optional<Text> text;
            std::optional<Motion> motion;
            std::function<void()> click;
        };

        /** Returns a checked mutable slot or throws for a stale identifier. */
        Slot &checked(Entity entity);

        /** Returns a checked immutable slot or throws for a stale identifier. */
        const Slot &checked(Entity entity) const;

        /** Invokes the optional gameplay callback with the current input snapshot. */
        void tick(const Input &input, float dt);

        /** Removes only entities and their tweens while preserving scene UI. */
        void clearEntities();

        /** Dispatches one left-click, allowing UI to consume it before world objects. */
        void dispatchClick(
            Vec2 screenPoint,
            int viewportWidth,
            int viewportHeight
        );

        /** Submits sorted world drawables, an overlay, and UI to the renderer. */
        void render(Renderer2D &renderer);

        std::string name_;
        std::uint64_t id_ = 0;
        std::vector<Slot> slots_;
        std::vector<std::uint32_t> free_;
        std::unordered_map<std::string, SaveValue> values_;
        AnimationSystem animations_;
        UIContext ui_;
        std::function<void(Canvas &)> overlay_;
        std::function<void(Scene &, const Input &, float)> updateCallback_;
        Color background_{0.075f, 0.09f, 0.13f, 1.0f};
        Vec2 camera_{};
        bool paused_ = false;
    };

    /** Returns the topmost eligible drawable under a world-space point. */
    Entity hitTest(
        const Scene &scene,
        Vec2 screenPoint,
        int viewportWidth,
        int viewportHeight
    );
} // namespace gd
