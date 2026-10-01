#include "gemedev/Application.hpp"

#include "scenes.hpp"

namespace flappy::game {
    void setup(gd::Application &app, gd::Scene &scene) {
        scene.setPaused(true);

        const gd::TextureHandle backgroundTexture = app.assets().texture(
            asset("textures/game/background.png")
        );

        const auto background = scene.createEntity("background");

        scene.add(background, gd::Transform{
            .kind = gd::TransformKind::Absolute,
            .position = {0.0f, 0.0f}
        });

        gd::Panorama panorama;
        panorama.texture = backgroundTexture;
        panorama.velocity = {200.0f, 0.0f};
        panorama.parallax = 1.0f;
        panorama.drivesCamera = true;
        panorama.fit = gd::PanoramaFit::FitHeight;

        scene.add(background, panorama);
    }
}
