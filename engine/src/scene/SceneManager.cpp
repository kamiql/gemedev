#include <gemedev/SceneManager.hpp>
#include <stdexcept>

namespace gd {
/** Creates a scene while preserving references to existing scenes. */
Scene& SceneManager::create(const std::string& name) {
    if (name.empty()) throw std::invalid_argument("Scene name must not be empty");
    if (scenes_.contains(name)) throw std::invalid_argument("Scene already exists: " + name);
    auto scene = std::make_unique<Scene>(name);
    Scene* result = scene.get();
    scenes_.emplace(name, std::move(scene));
    if (!active_) active_ = result;
    return *result;
}
/** Activates an existing scene by name. */
void SceneManager::activate(const std::string& name) {
    Scene* selected = find(name);
    if (!selected) throw std::out_of_range("Unknown scene: " + name);
    active_ = selected;
}
/** Returns the active scene or reports a missing scene. */
Scene& SceneManager::active() {
    if (!active_) throw std::logic_error("Create a scene before starting the application");
    return *active_;
}
/** Returns the active scene or reports a missing scene. */
const Scene& SceneManager::active() const {
    if (!active_) throw std::logic_error("Create a scene before starting the application");
    return *active_;
}
/** Looks up a named scene without changing active state. */
Scene* SceneManager::find(const std::string& name) const noexcept {
    const auto it = scenes_.find(name);
    return it == scenes_.end() ? nullptr : it->second.get();
}
}
