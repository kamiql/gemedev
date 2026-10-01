#pragma once
#include <gemedev/Assets.hpp>
#include <gemedev/Canvas.hpp>
#include <functional>
#include <string>
#include <vector>
#include <utility>

namespace gd {
/** A stable identifier for one UI element in a scene-local UI tree. */
using WidgetId = std::uint32_t;
/** A retained tree of panels, buttons, and labels in screen coordinates. */
class UIContext {
public:
    /** Adds a panel beneath a parent; positions are relative to that parent. */
    WidgetId panel(Rect bounds, WidgetId parent = 0, Color background = {0.15f, 0.19f, 0.25f, 0.96f});
    /** Adds a clickable button beneath a parent. */
    WidgetId button(std::string caption, Rect bounds, std::function<void()> callback, WidgetId parent = 0);
    /** Adds a non-interactive label beneath a parent. */
    WidgetId label(std::string caption, Vec2 position, WidgetId parent = 0);
    /** Sets the font shared by UI captions and labels. */
    void setFont(FontHandle font) { font_ = std::move(font); }
    /** Removes all UI elements while leaving the configured font intact. */
    void clear();
    /** Dispatches a pointer click to the topmost element; returns whether it was consumed. */
    bool handleClick(Vec2 point);
    /** Draws UI elements in insertion order, placing children relative to their parents. */
    void draw(Canvas& canvas) const;
private:
    enum class Kind { Panel, Button, Label };
    struct Widget {
        WidgetId id = 0;
        WidgetId parent = 0;
        Kind kind = Kind::Panel;
        Rect bounds{};
        std::string caption;
        std::function<void()> callback;
        Color background{};
    };
    /** Returns the screen-space origin of a widget, including every parent offset. */
    Vec2 absolutePosition(const Widget& widget) const;
    /** Finds an element by stable ID or throws if it does not exist. */
    const Widget& find(WidgetId id) const;
    std::vector<Widget> widgets_;
    WidgetId nextId_ = 1;
    FontHandle font_;
};
}
