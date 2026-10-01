#pragma once
#include <gemedev/Components.hpp>

namespace gd {
class Renderer2D;
/** A short-lived drawing facade for overlays and custom scene visuals. */
class Canvas {
public:
    /** Draws a rectangle, optionally sampling a texture across its full area. */
    void rect(Rect bounds, Color color = {}, TextureHandle texture = {});
    /** Draws a circular mask, optionally filled by a texture. */
    void circle(Rect bounds, Color color = {}, TextureHandle texture = {});
    /** Draws a triangle, optionally filled by a texture. */
    void triangle(Rect bounds, Color color = {}, TextureHandle texture = {});
    /** Draws text from a font atlas; text uses UTF-8 with unsupported glyphs replaced. */
    void text(Vec2 position, const std::string& value, FontHandle font, Color color = {});
private:
    friend class Application;
    friend class Scene;
    friend class UIContext;
    explicit Canvas(Renderer2D& renderer) : renderer_(renderer) {}
    Renderer2D& renderer_;
};
}
