#include "gemedev/Application.hpp"

#include "scenes.hpp"

namespace flappy::game {
    void setup(gd::Application &app, gd::Scene &scene) {
        const gd::TextureHandle backgroundTexture = app.assets().texture(
            asset("textures/game/background.png")
        );

        scene.setBackground(backgroundTexture);
    }
}
