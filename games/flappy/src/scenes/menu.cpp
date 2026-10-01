#include "gemedev/Application.hpp"

#include "scenes.hpp"

namespace flappy::menu {
    void setup(gd::Application &app, gd::Scene &scene) {
        const gd::TextureHandle backgroundTexture = app.assets().texture(
            asset("textures/game/background.png")
        );

        const gd::TextureHandle titleTexture = app.assets().texture(
            asset("textures/menu/title.png")
        );

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

        scene.onClick(title, [&app] {
            app.scenes().transitionTo(
                "game",
                gd::Transition::slideLeft(1.0f),
                [&app] {
                    if (auto* game = app.scenes().find("game")) {
                        game->setPaused(false);
                    }
                }
            );
        });
    }
}
