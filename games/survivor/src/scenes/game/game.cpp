#include "../scenes.hpp"
#include "game.hpp"

#include <iostream>

namespace survivor::game {
    static gd::Entity player;

    void setup(gd::Application& app, Context& context, Game& game, gd::Scene& scene) {

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

        scene.setUpdateCallback([&app](gd::Scene& scene, gd::Input input, float) {
            auto motion = gd::Motion {};
            if (input.keyDown(gd::Key::W)) {
                if (input.keyDown(gd::Key::Shift)) {
                    motion.velocity = motion.velocity + gd::Vec2 {
                        0, -25
                    };
                }

                motion.velocity = motion.velocity + gd::Vec2 {
                    0, -50
                };
            }
            if (input.keyDown(gd::Key::S)) {
                motion.velocity = motion.velocity + gd::Vec2 {
                    0, 50
                };
            }
            if (input.keyDown(gd::Key::A)) {
                motion.velocity = motion.velocity + gd::Vec2 {
                    -50, 0
                };
            }
            if (input.keyDown(gd::Key::D)) {
                motion.velocity = motion.velocity + gd::Vec2 {
                    50, 0
                };
            }

            scene.add(player, motion);
            const auto playerPosition = scene.transform(player)->position;
            scene.setCamera({
                playerPosition.x - app.middle().x,
                playerPosition.y - app.middle().y
            });
        });

        scene.setOverlay([&scene, &app](gd::Canvas canvas) {
            const gd::Vec2 camera = scene.camera();

            std::cout << camera.x << ' ' << camera.y << std::endl;

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
        });
    }
}
