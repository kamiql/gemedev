#include <iostream>

#include "gemedev/Application.hpp"

#include "scenes.hpp"

namespace flappy::game {
    static gd::Entity background;
    static gd::Entity player;

    void setup(gd::Application &app, gd::Scene &scene) {
        scene.setPaused(true);

        const gd::TextureHandle backgroundTexture = app.assets().texture(
            asset("textures/game/background.png")
        );

        background = scene.createEntity("background");
        scene.add(background, gd::Transform{
            .kind = gd::TransformKind::Absolute,
            .position = {0.0f, 0.0f}
        });

        gd::Panorama panorama;
        panorama.texture = backgroundTexture;
        panorama.parallax = 1.0f;
        panorama.drivesCamera = true;
        panorama.velocity = { 250.0f, 0.0f };
        panorama.fit = gd::PanoramaFit::FitHeight;
        scene.add(background, panorama);


        auto playerAnimation = scene.assets().animation(asset("textures/player"));
        player = scene.createEntity("player");
        scene.add(player, gd::Transform{
            .kind = gd::TransformKind::Relative,
            .position = { 0, 0 },
            .scale = { 0.075, 0.075 }
        });
        scene.add(player, gd::Sprite{});
        scene.add(player, gd::Animation {
            playerAnimation,
            "idle",
            0
        });

        scene.setUpdateCallback(
            [velocityY = 0.0f, &app, &scene, playerAnimation](
                gd::Scene& currentScene,
                const gd::Input& input,
                const float delta
            ) mutable {
                if (gd::Transform* t = currentScene.transform(player)) {
                    gd::Vec2 screenPos = t->position - currentScene.camera();

                    if (screenPos.y >= 1 || screenPos.y <= -1) {
                        currentScene.setPaused(true);

                        scene.clear();
                        setup(app, scene);

                        app.scenes().activate(
                            "menu"
                        );
                    }

                    if (gd::Animation* anim = currentScene.animation(player)) {
                        if (velocityY > 0.0f) {
                            anim->clip = "fall";
                        } else if (velocityY < 0.0f) {
                            anim->clip = "jump";
                        }
                    }
                }

                constexpr float gravityAcceleration = 1400.0f;
                constexpr float jumpVelocity = -600.0f;
                constexpr float terminalVelocity = 800.0f;

                if (input.mousePressed(gd::MouseButton::Left)) {
                    velocityY = jumpVelocity;
                }

                if (input.keyPressed(gd::Key::Space)) {
                    velocityY = jumpVelocity;
                }

                velocityY = std::min(
                    velocityY + gravityAcceleration * delta,
                    terminalVelocity
                );

                currentScene.add(player, gd::Motion{
                    {.x = 0, .y = -velocityY / 250},
                });
            }
        );
    }
}
