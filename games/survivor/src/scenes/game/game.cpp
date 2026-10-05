#include "../scenes.hpp"
#include "game.hpp"

#include <list>
#include <memory>

namespace survivor::game {
    static gd::Entity player;

    void setup(gd::Application& app, Context& context, Game& game, gd::Scene& scene) {
        auto movementQueue = std::make_shared<std::list<gd::Vec2>>();

        player = scene.createEntity("player");
        scene.add(player, gd::Transform {
            gd::TransformKind::Absolute,
            app.middle(),
            { 64.0f, 64.0f },
        });
        scene.add(player, gd::Shape {
            gd::ShapeKind::Circle,
            { 1.0f, 1.0f },
            {},
            { 1.0f, 1.0f, 1.0f, 1.0f }
        });

        scene.setUpdateCallback([&app, movementQueue, game](gd::Scene& scene, gd::Input input, float dt) mutable {
            auto motion = gd::Motion {};
            if (input.keyDown(gd::Key::W)) {
                motion.velocity = motion.velocity + gd::Vec2 {0, -game.speed};
                movementQueue->clear();
            }
            if (input.keyDown(gd::Key::S)) {
                motion.velocity = motion.velocity + gd::Vec2 {0, game.speed};
                movementQueue->clear();
            }
            if (input.keyDown(gd::Key::A)) {
                motion.velocity = motion.velocity + gd::Vec2 {-game.speed, 0};
                movementQueue->clear();
            }
            if (input.keyDown(gd::Key::D)) {
                motion.velocity = motion.velocity + gd::Vec2 {game.speed, 0};
                movementQueue->clear();
            }

            if (input.mousePressed(gd::MouseButton::Left)) {
                movementQueue->push_back(scene.camera() + input.mousePosition());
            }

            if (input.mousePressed(gd::MouseButton::Right)) {
                const auto [x, y] = scene.camera() + input.mousePosition();

                constexpr float radius = 12.0f;
                constexpr float radiusSquared = radius * radius;

                const auto it = std::ranges::find_if(
                    movementQueue->begin(),
                    movementQueue->end(),
                    [&](const gd::Vec2& target) {
                        const float dx = target.x - x;
                        const float dy = target.y - y;
                        return dx * dx + dy * dy <= radiusSquared;
                    }
                );

                if (it != movementQueue->end()) {
                    movementQueue->erase(it, movementQueue->end());
                }
            }

            const auto [px, py] = scene.transform(player)->position;

            if (!movementQueue->empty()) {
                const auto [x, y] = movementQueue->front();

                const float dx = x - px;
                const float dy = y - py;
                const float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < 1.0f) {
                    movementQueue->pop_front();
                } else {
                    float actualSpeed = game.speed;
                    if (dt > 0.0f && distance < game.speed * dt) {
                        actualSpeed = distance / dt;
                    }

                    motion.velocity = motion.velocity + gd::Vec2 {
                        (dx / distance) * actualSpeed,
                        (dy / distance) * actualSpeed
                    };
                }
            }

            scene.add(player, motion);

            const auto [x, y] = scene.transform(player)->position;
            scene.setCamera({
                x - app.middle().x,
                y - app.middle().y
            });
        });

        scene.setOverlay([&scene, &app, movementQueue](gd::Canvas canvas) {
            const gd::Vec2 camera = scene.camera();

            constexpr float spacing = 100.0f;

            const auto width  = static_cast<float>(app.width());
            const auto height = static_cast<float>(app.height());

            const float left   = camera.x - width / 2.0f;
            const float right  = camera.x + width / 2.0f;
            const float top    = camera.y - height / 2.0f;
            const float bottom = camera.y + height / 2.0f;

            const float startX = std::ceil(left / spacing) * spacing;
            const float startY = std::ceil(top / spacing) * spacing;

            for (float y = startY; y <= bottom; y += spacing) {
                for (float x = startX; x <= right; x += spacing) {
                    const gd::Vec2 screenPosition{
                        x - camera.x + width / 2.0f,
                        y - camera.y + height / 2.0f
                    };

                    canvas.circle(
                        {
                            screenPosition,
                            { 4.0f, 4.0f },
                        },
                        { 0.0f, 0.2f, 1.0f, 1.0f }
                    );
                }
            }

            for (const auto& queued : *movementQueue) {
                const auto pos = queued - scene.camera();

                canvas.circle(
                    {
                        pos,
                        {16.0f, 16.0f}
                    },
                    {
                        1.0f, 1.0f, 0.0f, 1.0f
                    }
                );
            }

            if (!movementQueue->empty()) {
                gd::Vec2 previous = scene.transform(player)->position;

                for (const auto& target : *movementQueue) {
                    canvas.line(
                        (previous + gd::Vec2 {8.0f, 8.0f}) - camera,
                        (target + gd::Vec2 {8.0f, 8.0f}) - camera,
                        {1.0f, 1.0f, 0.0f, 1.0f}
                    );

                    previous = target;
                }
            }
        });
    }
}
