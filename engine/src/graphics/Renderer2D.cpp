#include <GL/glew.h>

#include "graphics/Renderer2D.hpp"

#include <gemedev/Canvas.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

namespace gd {
namespace {

/** Compiles a GLSL stage and reports the shader compiler's diagnostics. */
unsigned compileShader(unsigned type, const char* source) {
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (!status) {
        std::array<char, 2048> log{};
        glGetShaderInfoLog(
            shader,
            static_cast<int>(log.size()),
            nullptr,
            log.data()
        );

        glDeleteShader(shader);
        throw std::runtime_error(
            "GLSL shader compile failed: " + std::string(log.data())
        );
    }

    return shader;
}

/** Decodes one UTF-8 code point or yields a replacement character. */
char32_t nextCodePoint(const std::string& text, std::size_t& index) {
    const auto first = static_cast<unsigned char>(text[index++]);

    if (first < 0x80) {
        return first;
    }

    const int trailing =
        first >= 0xF0 && first <= 0xF4 ? 3 :
        first >= 0xE0 && first <= 0xEF ? 2 :
        first >= 0xC2 && first <= 0xDF ? 1 :
        0;

    if (!trailing ||
        index + static_cast<std::size_t>(trailing) > text.size()) {
        return U'?';
    }

    char32_t code = first & ((1 << (6 - trailing)) - 1);

    for (int n = 0; n < trailing; ++n) {
        const unsigned char part =
            static_cast<unsigned char>(text[index]);

        if ((part & 0xC0) != 0x80) {
            return U'?';
        }

        ++index;
        code = (code << 6) | (part & 0x3F);
    }

    return code;
}

/**
 * Converts an NDC anchor into the object's top-left pixel position.
 * The object's center is placed at the requested anchor.
 */
Vec2 ndcAnchorToTopLeft(Vec2 ndc, Vec2 size, int width, int height) {
    const float anchorX =
        (ndc.x + 1.0f) * 0.5f * static_cast<float>(width);

    // Screen coordinates start at the top, so NDC Y is inverted.
    const float anchorY =
        (1.0f - ndc.y) * 0.5f * static_cast<float>(height);

    return {
        anchorX - size.x * 0.5f,
        anchorY - size.y * 0.5f
    };
}

} // namespace

/** Initializes an editable quad-based OpenGL renderer. */
Renderer2D::Renderer2D() {
    static constexpr auto vertex = R"GLSL(
        #version 330 core

        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec2 aLocal;

        uniform vec2 uScreen;

        out vec2 vUV;
        out vec2 vLocal;

        void main() {
            vec2 clip = aPosition / uScreen * 2.0 - 1.0;
            gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
            vUV = aUV;
            vLocal = aLocal;
        }
    )GLSL";

    static constexpr auto fragment = R"GLSL(
        #version 330 core

        in vec2 vUV;
        in vec2 vLocal;

        uniform sampler2D uTexture;
        uniform vec4 uColor;
        uniform int uShapeKind;
        uniform int uOutline;
        uniform vec2 uPixelSize;
        uniform float uOutlineWidth;

        out vec4 FragColor;

        void main() {
            // ShapeKind: Rectangle = 0, Circle = 1, Triangle = 2.
            if (uShapeKind == 1 &&
                length(vLocal - vec2(0.5)) > 0.5) {
                discard;
            }

            if (uOutline != 0) {
                float edgeDistance;

                if (uShapeKind == 1) {
                    // Circle edge, with approximately pixel-sized thickness.
                    edgeDistance =
                        (0.5 - length(vLocal - vec2(0.5))) *
                        min(uPixelSize.x, uPixelSize.y);
                } else if (uShapeKind == 2) {
                    // Triangle vertices are (0,0), (1,0), and (0,1).
                    float leftEdge =
                        vLocal.x * uPixelSize.x;
                    float topEdge =
                        vLocal.y * uPixelSize.y;

                    float diagonalEdge =
                        (1.0 - vLocal.x - vLocal.y) /
                        length(vec2(
                            1.0 / uPixelSize.x,
                            1.0 / uPixelSize.y
                        ));

                    edgeDistance =
                        min(leftEdge, min(topEdge, diagonalEdge));
                } else {
                    // Rectangle and sprite quad.
                    edgeDistance = min(
                        min(
                            vLocal.x * uPixelSize.x,
                            (1.0 - vLocal.x) * uPixelSize.x
                        ),
                        min(
                            vLocal.y * uPixelSize.y,
                            (1.0 - vLocal.y) * uPixelSize.y
                        )
                    );
                }

                // Keep only pixels close to the inside edge.
                if (edgeDistance > uOutlineWidth) {
                    discard;
                }

                FragColor = vec4(1.0, 0.0, 0.0, uColor.a);
                return;
            }

            FragColor = texture(uTexture, vUV) * uColor;
        }
    )GLSL";

    const unsigned vs = compileShader(GL_VERTEX_SHADER, vertex);
    unsigned fs = 0;

    try {
        fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int status = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);

    if (!status) {
        std::array<char, 2048> log{};
        glGetProgramInfoLog(
            program_,
            static_cast<int>(log.size()),
            nullptr,
            log.data()
        );

        glDeleteProgram(program_);
        program_ = 0;

        throw std::runtime_error(
            "GLSL link failed: " + std::string(log.data())
        );
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * 36,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(0)
    );
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))
    );
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(4 * sizeof(float))
    );

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glGenTextures(1, &white_);
    glBindTexture(GL_TEXTURE_2D, white_);

    const unsigned char pixel[4] = {255, 255, 255, 255};

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixel
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(program_);
    glUniform1i(glGetUniformLocation(program_, "uTexture"), 0);
}

/** Releases shader, buffer, vertex array, and fallback texture. */
Renderer2D::~Renderer2D() {
    if (white_) {
        glDeleteTextures(1, &white_);
    }

    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
    }

    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
    }

    if (program_) {
        glDeleteProgram(program_);
    }
}

/** Prepares the GL framebuffer and a logical-size orthographic projection. */
void Renderer2D::begin(
    int framebufferWidth,
    int framebufferHeight,
    int windowWidth,
    int windowHeight,
    Color clear
) {
    framebufferWidth_ = framebufferWidth;
    framebufferHeight_ = framebufferHeight;
    windowWidth_ = std::max(1, windowWidth);
    windowHeight_ = std::max(1, windowHeight);

    drawOffset_ = {};
    opacity_ = 1.0f;
    glViewport(0, 0, framebufferWidth_, framebufferHeight_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(clear.r, clear.g, clear.b, clear.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glUniform2f(
        glGetUniformLocation(program_, "uScreen"),
        static_cast<float>(windowWidth_),
        static_cast<float>(windowHeight_)
    );
}

/** Draws a primitive using pixels or relative NDC coordinates. */
void Renderer2D::draw(
    ShapeKind kind,
    const Transform& transform,
    Vec2 size,
    Color tint,
    const TextureHandle& texture,
    bool outline
) {
    Transform resolved = transform;

    if (transform.kind == TransformKind::Relative) {
        // In Relative mode, scale is expressed as a fraction of the viewport.
        size = {
            transform.scale.x * static_cast<float>(windowWidth_),
            transform.scale.y * static_cast<float>(windowHeight_)
        };

        resolved.position = ndcAnchorToTopLeft(
            transform.position,
            size,
            windowWidth_,
            windowHeight_
        );

        // Relative scale was already applied to size.
        resolved.scale = {1.0f, 1.0f};
        resolved.kind = TransformKind::Absolute;
    }

    drawQuad(
        kind,
        resolved,
        size,
        tint,
        texture ? texture->id_ : white_,
        {0.0f, 0.0f},
        {1.0f, 1.0f},
        outline
    );
}

/** Expands geometry into dynamic vertices and draws the primitive and its outline. */
void Renderer2D::drawQuad(
    ShapeKind kind,
    const Transform& transform,
    Vec2 size,
    Color color,
    unsigned texture,
    Vec2 uv0,
    Vec2 uv1,
    bool outline
) {
    const float w = size.x * transform.scale.x;
    const float h = size.y * transform.scale.y;
    Transform translated = transform;
    translated.position = translated.position + drawOffset_;
    color.a *= opacity_;

    if (w == 0.0f || h == 0.0f || color.a <= 0.0f) {
        return;
    }

    const float radians =
        transform.rotationDegrees * 0.017453292519943295f;
    const float c = std::cos(radians);
    const float s = std::sin(radians);

    const Vec2 corners[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    const int indices[6] = {0, 1, 2, 0, 2, 3};
    const int triangle[3] = {0, 1, 3};

    std::array<float, 36> vertices{};
    const int count = kind == ShapeKind::Triangle ? 3 : 6;

    for (int i = 0; i < count; ++i) {
        const Vec2 local =
            corners[kind == ShapeKind::Triangle ? triangle[i] : indices[i]];

        const float x = (local.x - 0.5f) * w;
        const float y = (local.y - 0.5f) * h;
        const auto j = static_cast<std::size_t>(i * 6);

        vertices[j] =
            translated.position.x + size.x * 0.5f + x * c - y * s;
        vertices[j + 1] =
            translated.position.y + size.y * 0.5f + x * s + y * c;

        vertices[j + 2] = uv0.x + (uv1.x - uv0.x) * local.x;
        vertices[j + 3] = uv0.y + (uv1.y - uv0.y) * local.y;
        vertices[j + 4] = local.x;
        vertices[j + 5] = local.y;
    }

    glUseProgram(program_);

    glUniform1i(
        glGetUniformLocation(program_, "uShapeKind"),
        static_cast<int>(kind)
    );

    glUniform2f(
        glGetUniformLocation(program_, "uPixelSize"),
        std::abs(w),
        std::abs(h)
    );

    glUniform1f(
        glGetUniformLocation(program_, "uOutlineWidth"),
        2.0f
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<std::size_t>(count * 6) * sizeof(float),
        vertices.data()
    );

    const auto drawPass = [&](bool outlinePass) {
        glUniform1i(
            glGetUniformLocation(program_, "uOutline"),
            outlinePass ? 1 : 0
        );

        if (outlinePass) {
            glUniform4f(
                glGetUniformLocation(program_, "uColor"),
                1.0f, 0.0f, 0.0f, color.a
            );
        } else {
            glUniform4f(
                glGetUniformLocation(program_, "uColor"),
                color.r, color.g, color.b, color.a
            );
        }

        glDrawArrays(GL_TRIANGLES, 0, count);
    };

    // Draw the entity first, then its red outline.
    drawPass(false);

    if (outline) {
        drawPass(true);
    }
}

/** Draws text at an absolute pixel position. */
void Renderer2D::drawText(
    Vec2 position,
    const std::string& value,
    const FontHandle& font,
    Color tint
) {
    if (!font) {
        return;
    }

    Vec2 pen = position;

    for (std::size_t i = 0; i < value.size();) {
        const char32_t point = nextCodePoint(value, i);

        if (point == U'\n') {
            pen.x = position.x;
            pen.y += font->pixelHeight_ * 1.25f;
            continue;
        }

        auto it = font->glyphs_.find(point);

        if (it == font->glyphs_.end()) {
            it = font->glyphs_.find(U'?');
        }

        if (it == font->glyphs_.end()) {
            continue;
        }

        const auto& glyph = it->second;

        const Transform glyphTransform{
            TransformKind::Absolute,
            {
                pen.x + glyph.bearing.x,
                pen.y + font->pixelHeight_ - glyph.bearing.y
            }
        };

        drawQuad(
            ShapeKind::Rectangle,
            glyphTransform,
            glyph.size,
            tint,
            font->id_,
            glyph.uv0,
            glyph.uv1,
            false
        );

        pen.x += glyph.advance;
    }
}

/** Draws text using absolute pixels or relative NDC coordinates. */
void Renderer2D::drawText(
    const Transform& transform,
    const std::string& value,
    const FontHandle& font,
    Color tint
) {
    if (!font) {
        return;
    }

    if (transform.kind == TransformKind::Absolute) {
        drawText(transform.position, value, font, tint);
        return;
    }

    float lineWidth = 0.0f;
    float maxWidth = 0.0f;
    int lineCount = 1;

    for (std::size_t i = 0; i < value.size();) {
        const char32_t point = nextCodePoint(value, i);

        if (point == U'\n') {
            maxWidth = std::max(maxWidth, lineWidth);
            lineWidth = 0.0f;
            ++lineCount;
            continue;
        }

        auto it = font->glyphs_.find(point);

        if (it == font->glyphs_.end()) {
            it = font->glyphs_.find(U'?');
        }

        if (it != font->glyphs_.end()) {
            lineWidth += it->second.advance;
        }
    }

    maxWidth = std::max(maxWidth, lineWidth);

    const float lineHeight = font->pixelHeight_ * 1.25f;
    const float textHeight =
        font->pixelHeight_ +
        static_cast<float>(lineCount - 1) * lineHeight;

    const Vec2 textTopLeft = ndcAnchorToTopLeft(
        transform.position,
        {maxWidth, textHeight},
        windowWidth_,
        windowHeight_
    );

    drawText(textTopLeft, value, font, tint);
}

/** Draws a full-window background texture without an outline. */
void Renderer2D::drawBackground(const TextureHandle& texture) {
    if (!texture) {
        return;
    }

    draw(
        ShapeKind::Rectangle,
        Transform{
            TransformKind::Absolute,
            {0.0f, 0.0f}
        },
        {
            static_cast<float>(windowWidth_),
            static_cast<float>(windowHeight_)
        },
        {1.0f, 1.0f, 1.0f, 1.0f},
        texture,
        false
    );
}

/** Sets a global alpha multiplier, clamped to the normalized range. */
void Renderer2D::setOpacity(float opacity) noexcept {
    opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

/** Draws a viewport-sized color rectangle using the current transition state. */
void Renderer2D::drawSolidBackground(Color color) {
    draw(ShapeKind::Rectangle, Transform{TransformKind::Absolute, {0.0f, 0.0f}},
         {static_cast<float>(windowWidth_), static_cast<float>(windowHeight_)},
         color, {}, false);
}

/** Draws fitted, horizontally repeated copies across the viewport. */
void Renderer2D::drawPanorama(
    const TextureHandle& texture,
    Vec2 offset,
    Vec2 scale,
    Color tint,
    PanoramaFit fit
) {
    if (!texture) return;

    const Vec2 imageSize = texture->size();
    const float viewportWidth = static_cast<float>(std::max(1, windowWidth_));
    const float viewportHeight = static_cast<float>(std::max(1, windowHeight_));
    if (imageSize.x <= 0.0f || imageSize.y <= 0.0f) return;

    const float fitX = viewportWidth / imageSize.x;
    const float fitY = viewportHeight / imageSize.y;
    float baseScaleX = fitX;
    float baseScaleY = fitY;

    switch (fit) {
        case PanoramaFit::Stretch:
            break;
        case PanoramaFit::Cover: {
            const float uniform = std::max(fitX, fitY);
            baseScaleX = uniform;
            baseScaleY = uniform;
            break;
        }
        case PanoramaFit::Contain: {
            const float uniform = std::min(fitX, fitY);
            baseScaleX = uniform;
            baseScaleY = uniform;
            break;
        }
        case PanoramaFit::FitHeight:
            baseScaleX = baseScaleY = fitY;
            break;
    }

    const Vec2 tileSize{
        imageSize.x * baseScaleX * std::abs(scale.x),
        imageSize.y * baseScaleY * std::abs(scale.y)
    };
    if (tileSize.x <= 0.0f || tileSize.y <= 0.0f) return;

    const auto firstTile = [](float phase, float extent) {
        float position = std::fmod(phase, extent);
        if (position > 0.0f) position -= extent;
        return position;
    };
    const float startX = firstTile(offset.x, tileSize.x);
    // Center fitted/cropped rows vertically; no vertical repetition is needed
    // for a horizontally scrolling panorama.
    const float y = (viewportHeight - tileSize.y) * 0.5f + offset.y;

    for (float x = startX; x < viewportWidth; x += tileSize.x) {
        draw(
            ShapeKind::Rectangle,
            Transform{TransformKind::Absolute, {x, y}},
            tileSize,
            tint,
            texture,
            false
        );
    }
}

/** Draws a screen-space rectangle through the internal renderer. */
void Canvas::rect(Rect bounds, Color color, TextureHandle texture) {
    renderer_.draw(
        ShapeKind::Rectangle,
        Transform{TransformKind::Absolute, bounds.position},
        bounds.size,
        color,
        texture,
        false
    );
}

/** Draws a screen-space textured circle. */
void Canvas::circle(Rect bounds, Color color, TextureHandle texture) {
    renderer_.draw(
        ShapeKind::Circle,
        Transform{TransformKind::Absolute, bounds.position},
        bounds.size,
        color,
        texture,
        false
    );
}

/** Draws a screen-space textured triangle. */
void Canvas::triangle(Rect bounds, Color color, TextureHandle texture) {
    renderer_.draw(
        ShapeKind::Triangle,
        Transform{TransformKind::Absolute, bounds.position},
        bounds.size,
        color,
        texture,
        false
    );
}

/** Draws text at the specified screen-space position. */
void Canvas::text(
    Vec2 position,
    const std::string& value,
    FontHandle font,
    Color color
) {
    renderer_.drawText(position, value, font, color);
}

} // namespace gd