#pragma once

#include <string>

namespace survivor::game {
    struct Game {
        std::string name;
        /* In pixels */
        float speed = 75;
    };
}
