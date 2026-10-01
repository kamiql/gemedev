#include <gemedev/UI.hpp>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
    std::vector<std::string> wrapText(
        const std::string &text,
        const gd::Font &font,
        float maxWidth
    ) {
        std::vector<std::string> lines;
        std::istringstream paragraphs(text);
        std::string paragraph;

        while (std::getline(paragraphs, paragraph)) {
            std::istringstream words(paragraph);
            std::string word;
            std::string line;
            bool hadWord = false;

            while (words >> word) {
                hadWord = true;

                if (font.textWidth(word) > maxWidth) {
                    if (!line.empty()) {
                        lines.push_back(line);
                        line.clear();
                    }

                    std::string part;
                    for (char character: word) {
                        std::string candidate = part + character;

                        if (!part.empty() && font.textWidth(candidate) > maxWidth) {
                            lines.push_back(part);
                            part.clear();
                        }

                        part += character;
                    }

                    line = std::move(part);
                    continue;
                }

                const std::string candidate =
                        line.empty() ? word : line + " " + word;

                if (!line.empty() && font.textWidth(candidate) > maxWidth) {
                    lines.push_back(line);
                    line = word;
                } else {
                    line = candidate;
                }
            }

            if (!line.empty()) {
                lines.push_back(line);
            } else if (!hadWord) {
                lines.emplace_back();
            }
        }

        return lines;
    }
} // namespace

namespace gd {
    /** Resolves a widget ID to the element in this UI tree. */
    const UIContext::Widget &UIContext::find(WidgetId id) const {
        const auto it = std::find_if(widgets_.begin(), widgets_.end(),
                                     [id](const Widget &widget) { return widget.id == id; });
        if (it == widgets_.end()) throw std::out_of_range("Unknown parent widget ID");
        return *it;
    }

    /** Resolves the accumulated offset of a widget's parent chain. */
    Vec2 UIContext::absolutePosition(const Widget &widget) const {
        Vec2 origin = widget.bounds.position;
        WidgetId parent = widget.parent;
        while (parent != 0) {
            const auto &ancestor = find(parent);
            origin = origin + ancestor.bounds.position;
            parent = ancestor.parent;
        }
        return origin;
    }

    /** Creates a colored panel beneath an existing parent. */
    WidgetId UIContext::panel(Rect bounds, WidgetId parent, Color background) {
        if (parent != 0) find(parent);
        const auto id = nextId_++;
        widgets_.push_back({id, parent, Kind::Panel, bounds, {}, {}, background});
        return id;
    }

    /** Creates an interactive button beneath an existing parent. */
    WidgetId UIContext::button(std::string caption, Rect bounds, std::function<void()> callback, WidgetId parent) {
        if (parent != 0) find(parent);
        const auto id = nextId_++;
        widgets_.push_back({
            id, parent, Kind::Button, bounds, std::move(caption), std::move(callback), {0.10f, 0.43f, 0.76f, 1.0f}
        });
        return id;
    }

    /** Creates a text-only, pointer-transparent label. */
    WidgetId UIContext::label(std::string caption, Vec2 position, WidgetId parent) {
        if (parent != 0) find(parent);
        const auto id = nextId_++;
        widgets_.push_back({id, parent, Kind::Label, {position, {}}, std::move(caption), {}, {}});
        return id;
    }

    /** Drops widgets while keeping their IDs from being silently reused. */
    void UIContext::clear() { widgets_.clear(); }
    /** Dispatches clicks in reverse draw order and consumes clicks over panels. */
    bool UIContext::handleClick(Vec2 point) {
        for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
            if (it->kind == Kind::Label) continue;
            const Vec2 origin = absolutePosition(*it);
            if (point.x < origin.x || point.y < origin.y || point.x > origin.x + it->bounds.size.x || point.y > origin.y
                + it->bounds.size.y)
                continue;
            auto callback = it->callback;
            if (callback) callback();
            return true;
        }
        return false;
    }

    /** Draws panels, buttons, and labels without exposing the renderer. */
    void UIContext::draw(Canvas &canvas) const {
        for (const auto &widget: widgets_) {
            const Vec2 origin = absolutePosition(widget);

            if (widget.kind != Kind::Label) {
                canvas.rect(
                    {origin, widget.bounds.size},
                    widget.background
                );
            }

            if (!font_ || widget.caption.empty()) continue;

            const Color textColor{1.0f, 1.0f, 1.0f, 1.0f};

            if (widget.kind != Kind::Button) {
                canvas.text(origin, widget.caption, font_, textColor);
                continue;
            }

            constexpr float horizontalPadding = 12.0f;
            constexpr float lineSpacing = 4.0f;

            const float availableWidth = std::max(
                1.0f,
                widget.bounds.size.x - 2.0f * horizontalPadding
            );

            const auto lines = wrapText(
                widget.caption,
                *font_,
                availableWidth
            );

            const float lineHeight =
                    static_cast<float>(font_->pixelHeight()) + lineSpacing;

            const float blockHeight =
                    static_cast<float>(lines.size()) * lineHeight - lineSpacing;

            float y = origin.y +
                      (widget.bounds.size.y - blockHeight) / 2.0f;

            for (const auto &line: lines) {
                const float x = origin.x +
                                (widget.bounds.size.x - font_->textWidth(line)) / 2.0f;

                canvas.text({x, y}, line, font_, textColor);
                y += lineHeight;
            }
        }
    }
}
