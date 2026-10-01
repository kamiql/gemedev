#pragma once
#include <cstdint>

namespace gd {
/** A generation-checked identifier owned by exactly one scene. */
struct Entity {
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0;
    std::uint64_t sceneId = 0;
    /** Checks whether two identifiers refer to the same slot and generation. */
    friend bool operator==(Entity a, Entity b) = default;
};
}
