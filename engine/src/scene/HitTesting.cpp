#include <gemedev/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace gd {

/** Finds the topmost clickable sprite or primitive at a screen-space point. */
Entity hitTest(
    const Scene& scene,
    Vec2 screenPoint,
    int viewportWidth,
    int viewportHeight
) {
    struct Candidate {
        int layer;
        std::uint32_t index;
        int kind;
    };

    std::vector<Candidate> candidates;

    for (std::uint32_t i = 0; i < scene.slots_.size(); ++i) {
        const auto& slot = scene.slots_[i];

        if (!slot.alive || !slot.transform || !slot.click) {
            continue;
        }

        if (slot.sprite && slot.sprite->tint.a > 0.0f) {
            candidates.push_back({slot.sprite->layer, i, 0});
        }

        if (slot.shape && slot.shape->tint.a > 0.0f) {
            candidates.push_back({slot.shape->layer, i, 1});
        }
    }

    std::stable_sort(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.layer < b.layer;
        }
    );

    viewportWidth = std::max(1, viewportWidth);
    viewportHeight = std::max(1, viewportHeight);

    // Test in reverse draw order, so the visually topmost object wins.
    for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
        const auto& slot = scene.slots_[it->index];
        const auto& transform = *slot.transform;
        const bool isShape = it->kind == 1;

        Vec2 size{};
        Vec2 position = transform.position;
        Vec2 point = screenPoint;
        Vec2 scale = transform.scale;

        if (transform.kind == TransformKind::Relative) {
            // Relative scale is a fraction of the viewport size.
            size = {
                transform.scale.x * static_cast<float>(viewportWidth),
                transform.scale.y * static_cast<float>(viewportHeight)
            };

            // Relative transforms are screen-anchored, so camera movement
            // does not affect either their render position or hit test.
            const float anchorX =
                (transform.position.x + 1.0f) * 0.5f *
                static_cast<float>(viewportWidth);

            const float anchorY =
                (1.0f - transform.position.y) * 0.5f *
                static_cast<float>(viewportHeight);

            position = {
                anchorX - size.x * 0.5f,
                anchorY - size.y * 0.5f
            };

            // Relative scale has already been applied to size.
            scale = {1.0f, 1.0f};
        } else {
            // Absolute transforms are world-space pixels. Rendering subtracts
            // the camera, so convert the screen click back to world space.
            point = screenPoint + scene.camera_;

            if (isShape) {
                size = slot.shape->size;
            } else {
                const Vec2 textureSize = slot.sprite->texture
                    ? slot.sprite->texture->size()
                    : Vec2{64.0f, 64.0f};

                // Match Scene::render: the callback receives texture size.
                size = slot.sprite->size(textureSize);
            }
        }

        const float w = size.x * scale.x;
        const float h = size.y * scale.y;

        if (w == 0.0f || h == 0.0f) {
            continue;
        }

        const float radians =
            transform.rotationDegrees * 0.017453292519943295f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        // Match Renderer2D::drawQuad: the geometry is centered at
        // position + unscaled size / 2.
        const float centerX = position.x + size.x * 0.5f;
        const float centerY = position.y + size.y * 0.5f;

        const float dx = point.x - centerX;
        const float dy = point.y - centerY;

        // Inverse-rotate the point into the object's local space.
        const float localX = dx * c + dy * s;
        const float localY = -dx * s + dy * c;

        const float x = localX / w + 0.5f;
        const float y = localY / h + 0.5f;

        if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f) {
            continue;
        }

        if (isShape && slot.shape->kind == ShapeKind::Circle) {
            const float nx = x - 0.5f;
            const float ny = y - 0.5f;

            if (nx * nx + ny * ny > 0.25f) {
                continue;
            }
        }

        if (isShape &&
            slot.shape->kind == ShapeKind::Triangle &&
            x + y > 1.0f) {
            continue;
        }

        return {it->index, slot.generation, scene.id_};
    }

    return {};
}

} // namespace gd