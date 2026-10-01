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

    /** How a panorama texture is scaled to the current viewport before tiling. */
    enum class PanoramaFit {
        /** Stretch one tile to the viewport width and height, like setBackground(). */
        Stretch,
        /** Preserve aspect ratio and cover the viewport; overflow is cropped. */
        Cover,
        /** Preserve aspect ratio and show the full image; letterbox space is clear color. */
        Contain,
        /** Preserve aspect ratio and fit the full image height (best for side-scrolling). */
        FitHeight
    };

    /**
     * An infinitely repeated, horizontally scrolling image attached to an entity.
     *
     * It does not need a Transform. `fit` determines the base tile size for the
     * current window; `scale` is an optional multiplier on that fitted size.
     * `velocity` is in logical pixels per second. With `drivesCamera` enabled,
     * it advances the scene camera (positive X progresses through the world, so
     * world objects move left on screen). Otherwise it scrolls only this texture.
     * `parallax` controls how much camera movement is applied to this layer:
     * 1.0 keeps it aligned with world-space entities, 0.0 locks it to the screen.
     */
    struct Panorama {
        TextureHandle texture{};
        Vec2 velocity{-40.0f, 0.0f};
        Vec2 scale{1.0f, 1.0f};
        Vec2 offset{};
        float parallax = 0.0f;
        Color tint{};
        int layer = -100;
        PanoramaFit fit = PanoramaFit::Stretch;
        bool drivesCamera = false;
    };
} // namespace gd
