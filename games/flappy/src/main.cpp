#include <filesystem>
#include <gemedev/Application.hpp>

#include "scenes/scenes.hpp"
#include "logic/HighScore.hpp"

std::int64_t highScore = 0;

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

    if (const auto* value = std::get_if<std::int64_t>(menu.value("highscore"))) {
        highScore = *value;
    }

    app.run();

    menu.setValue("highscore", highScore);
    app.saves().save(menu, "gemedev-savefile.json");

    return 0;
}