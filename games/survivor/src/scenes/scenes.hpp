#pragma once

#include <filesystem>
#include <string>
#include <gemedev/Application.hpp>

#include "../logic/Tiles.hpp"
#include "game/game.hpp"

namespace survivor {
    inline std::string asset(const std::string& relative) {
        const auto path = std::filesystem::absolute(
            std::filesystem::path(GEMEDEV_ASSET_DIR) / relative
        );
        return path.string();
    }

    struct Scenes {
        gd::Scene &menu;
        gd::Scene &settings;
        gd::Scene &game;
    };

    struct Context {
        Scenes scenes;
        std::vector<Tile> tiles;
        // app data, settings
    };

    namespace menu {
        void setup(gd::Application& app, Context& context, gd::Scene& scene);
    }

    namespace settings {
        void setup(gd::Application& app, Context& context, gd::Scene& scene);
    }

    namespace game {
        void setup(gd::Application& app, Context& context, Game& game, gd::Scene& scene);
    }

    // further global data variables / structs
}