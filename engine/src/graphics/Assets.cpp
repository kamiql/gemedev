#include <GL/glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <gemedev/Assets.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace gd {
/** Takes ownership of a newly uploaded OpenGL texture. */
Texture::Texture(unsigned id, int width, int height, std::string path)
    : id_(id), width_(width), height_(height), path_(std::move(path)) {}
/** Frees the texture on the current OpenGL context. */
Texture::~Texture() { if (id_) glDeleteTextures(1, &id_); }
/** Takes ownership of a newly uploaded glyph atlas. */
Font::Font(unsigned id, int pixelHeight, std::string path)
    : id_(id), pixelHeight_(pixelHeight), path_(std::move(path)) {}
/** Frees the glyph atlas on the current OpenGL context. */
Font::~Font() { if (id_) glDeleteTextures(1, &id_); }
/** Loads an image as RGBA and shares one GPU texture per live resource path. */
TextureHandle AssetCache::texture(const std::string& path) {
    if (auto cached = textures_[path].lock()) return cached;
    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!pixels) throw std::runtime_error("Cannot load texture '" + path + "': " + stbi_failure_reason());
    unsigned id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    auto loaded = TextureHandle(new Texture(id, width, height, path));
    textures_[path] = loaded;
    return loaded;
}
/** Rasterizes printable ASCII glyphs into one RGBA atlas per font and size. */
FontHandle AssetCache::font(const std::string& path, int pixelHeight) {
    if (pixelHeight < 8 || pixelHeight > 64) throw std::invalid_argument("Font height must be between 8 and 64 pixels");
    auto key = path + "#" + std::to_string(pixelHeight);
    if (auto cached = fonts_[key].lock()) return cached;
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library)) throw std::runtime_error("Could not initialize FreeType");
    FT_Face face = nullptr;
    if (FT_New_Face(library, path.c_str(), 0, &face)) {
        FT_Done_FreeType(library);
        throw std::runtime_error("Cannot load font: " + path);
    }
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixelHeight));
    const int cell = std::max(32, pixelHeight * 2);
    const int atlasWidth = 16 * cell;
    const int atlasHeight = 6 * cell;
    std::vector<unsigned char> pixels(static_cast<std::size_t>(atlasWidth * atlasHeight * 4), 0);
    auto loaded = FontHandle(new Font(0, pixelHeight, path));
    for (int code = 32; code <= 126; ++code) {
        if (FT_Load_Char(face, static_cast<FT_ULong>(code), FT_LOAD_RENDER)) continue;
        auto& glyph = face->glyph;
        int offsetX = (code - 32) % 16 * cell + 1;
        int offsetY = (code - 32) / 16 * cell + 1;
        const int width = static_cast<int>(glyph->bitmap.width);
        const int height = static_cast<int>(glyph->bitmap.rows);
        if (width >= cell - 1 || height >= cell - 1) continue;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int pitch = glyph->bitmap.pitch;
                const auto sourceRow = pitch < 0 ? (height - y - 1) * (-pitch) : y * pitch;
                const unsigned char alpha = glyph->bitmap.buffer[sourceRow + x];
                const auto pixel = static_cast<std::size_t>(((offsetY + y) * atlasWidth + offsetX + x) * 4);
                pixels[pixel] = 255;
                pixels[pixel + 1] = 255;
                pixels[pixel + 2] = 255;
                pixels[pixel + 3] = alpha;
            }
        }
        loaded->glyphs_[static_cast<char32_t>(code)] = Font::Glyph{
            {static_cast<float>(width), static_cast<float>(height)},
            {static_cast<float>(glyph->bitmap_left), static_cast<float>(glyph->bitmap_top)},
            {static_cast<float>(offsetX) / atlasWidth, static_cast<float>(offsetY) / atlasHeight},
            {static_cast<float>(offsetX + width) / atlasWidth, static_cast<float>(offsetY + height) / atlasHeight},
            static_cast<float>(glyph->advance.x) / 64.0f
        };
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    unsigned id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    loaded->id_ = id;
    fonts_[key] = loaded;
    return loaded;
}
/** Drops cache references without invalidating external shared handles. */
void AssetCache::clear() { textures_.clear(); fonts_.clear(); }
}
