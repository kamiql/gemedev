#pragma once

#include <gemedev/Assets.hpp>
#include <gemedev/Components.hpp>
#include <gemedev/Types.hpp>

#include <string>

namespace gd {

/** Implements primitive, textured-quad, outline, and glyph drawing in GL 3.3. */
class Renderer2D {
public:
    /** Creates shader, vertex buffers, vertex array, and white fallback texture. */
    Renderer2D();

    /** Deletes owned OpenGL objects while the context remains current. */
    ~Renderer2D();

    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    /** Configures the viewport and projection for a framebuffer and logical window. */
    void begin(
        int framebufferWidth,
        int framebufferHeight,
        int windowWidth,
        int windowHeight,
        Color clear
    );

    /** Draws a transformed primitive, optionally with a red outline. */
    void draw(
        ShapeKind kind,
        const Transform& transform,
        Vec2 size,
        Color tint,
        const TextureHandle& texture,
        bool outline = true
    );

    /** Draws text at an absolute pixel position. */
    void drawText(
        Vec2 position,
        const std::string& value,
        const FontHandle& font,
        Color tint
    );

    /** Draws text using the transform's position mode. */
    void drawText(
        const Transform& transform,
        const std::string& value,
        const FontHandle& font,
        Color tint
    );

    /** Draws a screen-space textured background without an outline. */
    void drawBackground(const TextureHandle& texture);

private:
    /** Draws a textured quad or triangle, optionally with a red outline. */
    void drawQuad(
        ShapeKind kind,
        const Transform& transform,
        Vec2 size,
        Color color,
        unsigned texture,
        Vec2 uv0,
        Vec2 uv1,
        bool outline
    );

    unsigned program_ = 0;
    unsigned vao_ = 0;
    unsigned vbo_ = 0;
    unsigned white_ = 0;

    int framebufferWidth_ = 1;
    int framebufferHeight_ = 1;
    int windowWidth_ = 1;
    int windowHeight_ = 1;
};

} // namespace gd