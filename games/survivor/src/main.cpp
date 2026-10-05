#include "gemedev/Application.hpp"
#include "scenes/scenes.hpp"

int main() {
    auto app = gd::Application({
        "Survivor",
        1280,
        720
    });

    auto &menu = app.scenes().create("menu");
    auto &settings = app.scenes().create("settings");
    auto &game = app.scenes().create("game");

    const auto scenes = survivor::Scenes {
        .menu = menu,
        .settings = settings,
        .game = game,
    };

    auto context = survivor::Context {
        scenes,
    };

    auto data = survivor::game::Game {};

    survivor::menu::setup(app, context, scenes.menu);
    survivor::settings::setup(app, context, scenes.settings);
    survivor::game::setup(app, context, data, game);

    app.scenes().activate(scenes.game.name());

    app.run();

    return 0;
}