#pragma once

#include <functional>
#include <gemedev/Assets.hpp>
#include <gemedev/Types.hpp>
#include <string>

#include "Entity.hpp"

namespace gd {
    enum class TransformKind {
        Absolute = 0,
        Relative = 1
    };

    /**
     * A 2D transform.
     *
     * Absolute: position is an absolute pixel/world position, as before.
     * Relative: position is an OpenGL-style NDC coordinate:
     *           (-1, -1) bottom-left, (0, 0) center, (1, 1) top-right.
     *           The object's center is anchored at that point.
     */
    struct Transform {
        TransformKind kind = TransformKind::Absolute;
        Vec2 position{};
        Vec2 scale{1.0f, 1.0f};
        float rotationDegrees = 0.0f;
    };

    using SpriteSizeCallback = std::function<Vec2(Vec2)>;

    struct Sprite {
        SpriteSizeCallback size = [](Vec2) {
            return Vec2{64.0f, 64.0f};
        };
        TextureHandle texture{};
        Color tint{};
        int layer = 0;
    };

    /** The supported editable geometric primitive kinds. */
    enum class ShapeKind {
        Rectangle,
        Circle,
        Triangle
    };

    /** A colored, optionally textured primitive. */
    struct Shape {
        ShapeKind kind = ShapeKind::Rectangle;
        Vec2 size{64.0f, 64.0f};
        TextureHandle texture{};
        Color tint{};
        int layer = 0;
    };

    /** A font-backed text component anchored at its transform position. */
    struct Text {
        std::string value;
        FontHandle font{};
        Color tint{};
        int layer = 0;
    };

    /** A world-unit-per-second linear movement component. */
    struct Motion {
        Vec2 velocity{};
    };
} // namespace gd
