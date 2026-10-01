#pragma once
#include <gemedev/Scene.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace gd {
/** Owns named scenes and tracks exactly one active scene. */
class SceneManager {
public:
    /** Creates a named scene and makes it active when none was active. */
    Scene& create(const std::string& name);
    /** Activates an existing scene or throws if it is missing. */
    void activate(const std::string& name);
    /** Returns the active scene or throws when there are no scenes. */
    Scene& active();
    /** Returns the active scene or throws when there are no scenes. */
    const Scene& active() const;
    /** Returns a scene by name or null if it does not exist. */
    Scene* find(const std::string& name) const noexcept;
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    Scene* active_ = nullptr;
};
}
