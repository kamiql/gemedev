#include <filesystem>
#include <gemedev/Application.hpp>

#include "scenes/scenes.hpp"

int main() {
    gd::Application app({
        .title = "Flappy",
        .width = 1280,
        .height = 720
    });

    app.scenes().create("game");

    auto &menu = app.scenes().create("menu");
    auto &death = app.scenes().create("death");

    app.scenes().activate(menu.name());

    flappy::menu::setup(app, menu);
    flappy::death::setup(app, death);

    app.run();
    return 0;
}