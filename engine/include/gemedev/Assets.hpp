#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gemedev/Types.hpp>

namespace gd {
class Renderer2D;
/** An owned OpenGL texture. Keep handles within the lifetime of Application. */
class Texture {
public:
    /** Releases the GPU texture while an OpenGL context is current. */
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    /** Returns the texture width in pixels. */
    int width() const noexcept { return width_; }
    /** Returns the texture height in pixels. */
    int height() const noexcept { return height_; }

    Vec2 size() const noexcept { return {static_cast<float>(width_), static_cast<float>(height_)}; }
    /** Returns the source path used to reload the asset. */
    const std::string& path() const noexcept { return path_; }
private:
    friend class AssetCache;
    friend class Renderer2D;
    Texture(unsigned id, int width, int height, std::string path);
    unsigned id_ = 0;
    int width_ = 0;
    int height_ = 0;
    std::string path_;
};
/** A cached font atlas that owns its OpenGL texture. */
class Font {
public:
    /** Releases the atlas texture while an OpenGL context is current. */
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    /** Returns the original font path. */
    const std::string& path() const noexcept { return path_; }
    /** Returns the requested font height in pixels. */
    int pixelHeight() const noexcept { return pixelHeight_; }

    /** Horizontaler Vorschub eines ASCII-Zeichens in Pixeln. */
    float glyphWidth(char character) const noexcept {
        const auto it = glyphs_.find(
            static_cast<char32_t>(static_cast<unsigned char>(character))
        );
        return it != glyphs_.end() ? it->second.advance : 0.0f;
    }

    /** Höhe der gerasterten Glyphe in Pixeln (bei Leerzeichen 0). */
    float glyphHeight(char character) const noexcept {
        const auto it = glyphs_.find(
            static_cast<char32_t>(static_cast<unsigned char>(character))
        );
        return it != glyphs_.end() ? it->second.size.y : 0.0f;
    }

    /** Breite eines ASCII-Textes anhand der Glyph-Vorschübe. */
    float textWidth(std::string_view text) const noexcept {
        float width = 0.0f;
        for (char character : text) {
            width += glyphWidth(character);
        }
        return width;
    }
private:
    friend class AssetCache;
    friend class Renderer2D;
    struct Glyph {
        Vec2 size{};
        Vec2 bearing{};
        Vec2 uv0{};
        Vec2 uv1{};
        float advance = 0.0f;
    };
    Font(unsigned id, int pixelHeight, std::string path);
    unsigned id_ = 0;
    int pixelHeight_ = 0;
    std::string path_;
    std::unordered_map<char32_t, Glyph> glyphs_;
};
using TextureHandle = std::shared_ptr<Texture>;
using FontHandle = std::shared_ptr<Font>;

/** One image and its display duration in a keyframe animation. */
struct AnimationKeyframe {
    std::string id;
    TextureHandle texture;
    std::uint32_t durationMs = 0;
};

/** A named, ordered sequence of keyframes loaded from keyframes.json. */
struct AnimationClip {
    std::string name;
    std::vector<AnimationKeyframe> keyframes;
};

/** A complete animation asset directory and its loaded textures. */
class AnimationAsset {
public:
    /** Returns the source directory passed to AssetCache::animation(). */
    const std::string& path() const noexcept { return path_; }
    /** Returns base.png when present; otherwise an empty handle. */
    const TextureHandle& baseTexture() const noexcept { return baseTexture_; }
    /** Finds a named clip, or returns null if the name is unknown. */
    const AnimationClip* findClip(const std::string& name) const noexcept {
        const auto it = clips_.find(name);
        return it != clips_.end() ? &it->second : nullptr;
    }
private:
    friend class AssetCache;
    explicit AnimationAsset(std::string path) : path_(std::move(path)) {}
    std::string path_;
    TextureHandle baseTexture_;
    std::unordered_map<std::string, AnimationClip> clips_;
};

using AnimationHandle = std::shared_ptr<AnimationAsset>;

/** Loads and caches image, font, and keyframe animation resources by path. Requires an active GL context. */
class AssetCache {
public:
    /** Creates an empty cache. */
    AssetCache() = default;
    /** Returns a cached texture, or loads and uploads it once. */
    TextureHandle texture(const std::string& path);
    /** Returns an ASCII font atlas cached by path and pixel height. */
    FontHandle font(const std::string& path, int pixelHeight = 24);
    /** Loads keyframes.json and its frame textures from an asset directory. */
    AnimationHandle animation(const std::string& path);
    /** Releases cache references; external handles may still own resources. */
    void clear();
private:
    std::unordered_map<std::string, std::weak_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::weak_ptr<Font>> fonts_;
    std::unordered_map<std::string, std::weak_ptr<AnimationAsset>> animations_;
};
}
