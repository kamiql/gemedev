#pragma once

#include <filesystem>
#include <string>
#include <gemedev/Application.hpp>

namespace flappy {
    inline std::string asset(const std::string& relative) {
        const auto path = std::filesystem::absolute(
            std::filesystem::path(GEMEDEV_ASSET_DIR) / relative
        );
        return path.string();
    }

    namespace menu {
        void setup(gd::Application& app, gd::Scene& scene);
    }

    namespace game {
        void setup(gd::Application& app, gd::Scene& scene);
    }

    namespace death {
        void setup(gd::Application& app, gd::Scene& scene);
    }
}