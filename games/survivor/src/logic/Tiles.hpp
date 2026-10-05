#pragma once
#include <string>

#include "gemedev/Application.hpp"
#include "gemedev/Assets.hpp"

struct Tile {
    std::string id;
    bool passable = true;
    std::int8_t danger = 0; // path finding stuff ig
    gd::TextureHandle texture;
};

std::vector<Tile> createTiles(gd::Application& app);