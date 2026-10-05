#include "gemedev/Application.hpp"

#include "scenes.hpp"
#include "../logic/HighScore.hpp"

namespace flappy::menu {
    void setup(gd::Application &app, gd::Scene &scene) {
        const gd::TextureHandle backgroundTexture = app.assets().texture(
            asset("textures/background.png")
        );

        const gd::TextureHandle titleTexture = app.assets().texture(
            asset("textures/title.png")
        );

        const gd::TextureHandle startTexture = app.assets().texture(
            asset("textures/start.png")
        );

        const gd::FontHandle font = app.assets().font(asset("textures/font/Silkscreen.ttf"));

        scene.setBackground(backgroundTexture);

        auto const title = scene.createEntity("title");
        scene.add(title, gd::Transform{
            .kind = gd::TransformKind::Relative,
            .position = {0.0f, 0.5f},
            .scale = [&app, &titleTexture] {
                const gd::Vec2 textureSize = titleTexture->size();
                const auto screenWidth = static_cast<float>(app.width());
                const auto screenHeight = static_cast<float>(app.height());

                const float fit = std::min({
                    screenWidth * 0.8f / textureSize.x,
                    screenHeight * 0.8f / textureSize.y
                });

                return gd::Vec2{
                    textureSize.x * fit / screenWidth,
                    textureSize.y * fit / screenHeight
                };
            }()
        });
        scene.add(title, gd::Sprite{
            .size = [](const gd::Vec2 textureSize) {
                return textureSize;
            },
            .texture = titleTexture,
        });

        auto const start = scene.createEntity("start");
        scene.add(start, gd::Transform{
            .kind = gd::TransformKind::Relative,
            .position = {0.0f, -0.3f},
            .scale = {
                0.25, 0.18
            }
        });
        scene.add(start, gd::Sprite{
            .size = [](const gd::Vec2 textureSize) {
                return textureSize;
            },
            .texture = startTexture,
        });

        auto playerAnimation = scene.assets().animation(asset("textures/player"));
        auto player = scene.createEntity("player");
        scene.add(player, gd::Transform{
            .kind = gd::TransformKind::Relative,
            .position = { -0.5, -0.3 },
            .scale = { 0.2, 0.2 }
        });
        scene.add(player, gd::Sprite{});
        scene.add(player, gd::Animation {
            playerAnimation,
            "jump",
            0
        });

        scene.setOverlay([font](gd::Canvas canvas) {
            canvas.text(
                {0.0f, 0.0f},
                "Highscore: " + std::to_string(highScore),
                font
            );
        });

        game::setup(app, *app.scenes().find("game"));
        scene.onClick(start, [&app] {
            app.scenes().transitionTo(
                "game",
                gd::Transition::slideLeft(0.5f),
                [&app] {
                    if (auto* game = app.scenes().find("game")) {
                        game->setPaused(false);
                    }
                }
            );
        });
    }
}
