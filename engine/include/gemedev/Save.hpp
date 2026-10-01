#pragma once

#include <gemedev/Assets.hpp>
#include <string>

namespace gd {
    class Scene;

    /** Reads and writes versioned JSON snapshots of persistent scene state. */
    class SaveService {
    public:
        /** Attaches the asset cache used to restore texture and font references. */
        explicit SaveService(AssetCache &assets)
            : assets_(assets) {
        }

        /** Writes a snapshot beside the target and replaces it without deleting old data first. */
        void save(const Scene &scene, const std::string &path) const;

        /** Validates a snapshot then replaces entities and persistent data in a scene. */
        void load(Scene &scene, const std::string &path) const;

    private:
        AssetCache &assets_;
    };
} // namespace gd
