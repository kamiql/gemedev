#include <gemedev/Save.hpp>
#include <gemedev/Scene.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace gd {
    namespace {
        using json = nlohmann::json;

        /** Serializes a world-space vector as two JSON numbers. */
        json toJson(Vec2 value) {
            return json::array({value.x, value.y});
        }

        /** Parses a pair of JSON numbers as a world-space vector. */
        Vec2 fromVec(const json &value) {
            if (!value.is_array() || value.size() != 2) {
                throw std::runtime_error("Expected a two-element vector");
            }

            return {
                value.at(0).get<float>(),
                value.at(1).get<float>()
            };
        }

        /** Serializes a non-premultiplied color into a four-element array. */
        json toJson(Color value) {
            return json::array({value.r, value.g, value.b, value.a});
        }

        /** Parses a four-channel floating-point color. */
        Color fromColor(const json &value) {
            if (!value.is_array() || value.size() != 4) {
                throw std::runtime_error("Expected a four-element color");
            }

            return {
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>(),
                value.at(3).get<float>()
            };
        }

        /** Converts a supported scalar variant into one JSON scalar. */
        json toJson(const SaveValue &value) {
            return std::visit(
                [](const auto &item) -> json {
                    return item;
                },
                value
            );
        }

        /** Rejects non-scalar JSON values from the persistent scene value map. */
        SaveValue fromValue(const json &value) {
            if (value.is_boolean()) {
                return value.get<bool>();
            }

            if (value.is_number_integer()) {
                return value.get<std::int64_t>();
            }

            if (value.is_number()) {
                return value.get<double>();
            }

            if (value.is_string()) {
                return value.get<std::string>();
            }

            throw std::runtime_error("Scene values must be JSON scalars");
        }
    } // namespace

    /** Writes versioned JSON containing only serializable scene components. */
    void SaveService::save(const Scene &scene, const std::string &path) const {
        json document = {
            {"version", 1},
            {"scene", scene.name_},
            {"camera", toJson(scene.camera_)},
            {"background", toJson(scene.background_)},
            {"paused", scene.paused_},
            {"values", json::object()},
            {"entities", json::array()}
        };

        for (const auto &[key, value]: scene.values_) {
            document["values"][key] = toJson(value);
        }

        for (const auto &slot: scene.slots_) {
            if (!slot.alive) {
                continue;
            }

            json entity = {
                {"name", slot.name}
            };

            if (slot.transform) {
                entity["transform"] = {
                    {"position", toJson(slot.transform->position)},
                    {"scale", toJson(slot.transform->scale)},
                    {"rotation", slot.transform->rotationDegrees},
                    {"kind", static_cast<int>(slot.transform->kind)}
                };
            }

            if (slot.sprite) {
                const Vec2 sizeInput = slot.transform
                                           ? slot.transform->scale
                                           : Vec2{1.0f, 1.0f};

                entity["sprite"] = {
                    {
                        "size",
                        toJson(slot.sprite->size(sizeInput))
                    },
                    {
                        "texture",
                        slot.sprite->texture
                            ? slot.sprite->texture->path()
                            : ""
                    },
                    {"tint", toJson(slot.sprite->tint)},
                    {"layer", slot.sprite->layer}
                };
            }

            if (slot.shape) {
                entity["shape"] = {
                    {"kind", static_cast<int>(slot.shape->kind)},
                    {"size", toJson(slot.shape->size)},
                    {
                        "texture",
                        slot.shape->texture
                            ? slot.shape->texture->path()
                            : ""
                    },
                    {"tint", toJson(slot.shape->tint)},
                    {"layer", slot.shape->layer}
                };
            }

            if (slot.text) {
                entity["text"] = {
                    {"value", slot.text->value},
                    {
                        "font",
                        slot.text->font
                            ? slot.text->font->path()
                            : ""
                    },
                    {
                        "fontSize",
                        slot.text->font
                            ? slot.text->font->pixelHeight()
                            : 24
                    },
                    {"tint", toJson(slot.text->tint)},
                    {"layer", slot.text->layer}
                };
            }

            if (slot.motion) {
                entity["motion"] = {
                    {"velocity", toJson(slot.motion->velocity)}
                };
            }

            document["entities"].push_back(std::move(entity));
        }

        const std::filesystem::path target(path);

        if (!target.parent_path().empty()) {
            std::filesystem::create_directories(target.parent_path());
        }

        const auto temporary = std::filesystem::path(path + ".tmp");

        {
            std::ofstream output(
                temporary,
                std::ios::binary | std::ios::trunc
            );

            if (!output) {
                throw std::runtime_error(
                    "Could not write save file: " + path
                );
            }

            output << document.dump(2) << '\n';
            output.flush();

            if (!output) {
                throw std::runtime_error(
                    "Failed while writing save file: " + path
                );
            }
        }

#ifdef _WIN32
        if (!MoveFileExW(
            temporary.wstring().c_str(),
            target.wstring().c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw std::runtime_error(
                "Could not replace save file (Windows error " +
                std::to_string(GetLastError()) +
                ")"
            );
        }
#else
        std::filesystem::rename(temporary, target);
#endif
    }

    /** Parses and loads all assets before replacing the current scene entities. */
    void SaveService::load(Scene &scene, const std::string &path) const {
        std::ifstream input(path, std::ios::binary);

        if (!input) {
            throw std::runtime_error("Could not read save file: " + path);
        }

        json document = json::parse(input);

        if (document.at("version").get<int>() != 1) {
            throw std::runtime_error("Unsupported save file version");
        }

        const auto camera = fromVec(document.at("camera"));
        const auto background = fromColor(document.at("background"));
        const bool paused = document.at("paused").get<bool>();

        std::unordered_map<std::string, SaveValue> values;

        for (auto it = document.at("values").begin();
             it != document.at("values").end();
             ++it) {
            values.emplace(it.key(), fromValue(it.value()));
        }

        struct Prepared {
            std::string name;
            std::optional<Transform> transform;
            std::optional<Sprite> sprite;
            std::optional<Shape> shape;
            std::optional<Text> text;
            std::optional<Motion> motion;
        };

        std::vector<Prepared> entries;
        std::unordered_set<std::string> names;

        for (const auto &item: document.at("entities")) {
            Prepared entry;
            entry.name = item.at("name").get<std::string>();

            if (!entry.name.empty() &&
                !names.insert(entry.name).second) {
                throw std::runtime_error(
                    "Duplicate entity name in save file: " + entry.name
                );
            }

            if (item.contains("transform")) {
                const auto &transformJson = item.at("transform");

                // Old save files without "kind" are treated as Absolute.
                const int transformKind =
                        transformJson.value("kind", 0);

                if (transformKind < 0 || transformKind > 1) {
                    throw std::runtime_error(
                        "Unknown transform kind in save file"
                    );
                }

                entry.transform = Transform{
                    static_cast<TransformKind>(transformKind),
                    fromVec(transformJson.at("position")),
                    fromVec(transformJson.at("scale")),
                    transformJson.at("rotation").get<float>()
                };
            }

            if (item.contains("sprite")) {
                const auto &spriteJson = item.at("sprite");
                const auto asset =
                        spriteJson.at("texture").get<std::string>();
                const Vec2 savedSize =
                        fromVec(spriteJson.at("size"));

                entry.sprite = Sprite{
                    [savedSize](Vec2) {
                        return savedSize;
                    },
                    asset.empty()
                        ? TextureHandle{}
                        : assets_.texture(asset),
                    fromColor(spriteJson.at("tint")),
                    spriteJson.at("layer").get<int>()
                };
            }

            if (item.contains("shape")) {
                const auto &shapeJson = item.at("shape");
                const auto shapeKind =
                        shapeJson.at("kind").get<int>();

                if (shapeKind < 0 || shapeKind > 2) {
                    throw std::runtime_error(
                        "Unknown shape kind in save file"
                    );
                }

                const auto asset =
                        shapeJson.at("texture").get<std::string>();

                entry.shape = Shape{
                    static_cast<ShapeKind>(shapeKind),
                    fromVec(shapeJson.at("size")),
                    asset.empty()
                        ? TextureHandle{}
                        : assets_.texture(asset),
                    fromColor(shapeJson.at("tint")),
                    shapeJson.at("layer").get<int>()
                };
            }

            if (item.contains("text")) {
                const auto &textJson = item.at("text");
                const auto pathToFont =
                        textJson.at("font").get<std::string>();

                entry.text = Text{
                    textJson.at("value").get<std::string>(),
                    pathToFont.empty()
                        ? FontHandle{}
                        : assets_.font(
                            pathToFont,
                            textJson.at("fontSize").get<int>()
                        ),
                    fromColor(textJson.at("tint")),
                    textJson.at("layer").get<int>()
                };
            }

            if (item.contains("motion")) {
                entry.motion = Motion{
                    fromVec(item.at("motion").at("velocity"))
                };
            }

            entries.push_back(std::move(entry));
        }

        scene.clearEntities();
        scene.values_ = std::move(values);
        scene.camera_ = camera;
        scene.background_ = background;
        scene.paused_ = paused;

        for (auto &entry: entries) {
            const Entity entity =
                    scene.createEntity(std::move(entry.name));

            if (entry.transform) {
                scene.add(entity, *entry.transform);
            }

            if (entry.sprite) {
                scene.add(entity, std::move(*entry.sprite));
            }

            if (entry.shape) {
                scene.add(entity, std::move(*entry.shape));
            }

            if (entry.text) {
                scene.add(entity, std::move(*entry.text));
            }

            if (entry.motion) {
                scene.add(entity, *entry.motion);
            }
        }
    }
} // namespace gd
