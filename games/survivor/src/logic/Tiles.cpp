#include "Tiles.hpp"

#include "../scenes/scenes.hpp"

std::vector<Tile> createTiles(gd::Application& app) {
    std::vector<Tile> tiles;

    tiles.push_back(Tile {
        "ground_dirt",
        true,
        0,
        app.assets().texture(survivor::asset(
            "textures/tiles/dirt.png"
        ))
    });

    return tiles;
}
