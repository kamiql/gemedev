#pragma once
#include <cstdint>
#include <string>
#include <variant>

namespace gd {
/** A two-dimensional vector expressed in logical screen or world units. */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};
/** Adds two vectors component-wise. */
inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
/** Subtracts two vectors component-wise. */
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
/** Scales a vector by a scalar. */
inline Vec2 operator*(Vec2 v, float factor) { return {v.x * factor, v.y * factor}; }
/** A rectangular region whose origin is its upper-left corner. */
struct Rect {
    Vec2 position{};
    Vec2 size{};
};
/** A non-premultiplied RGBA color with channels in the range 0 to 1. */
struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};
/** A JSON-compatible scalar saved as part of scene state. */
using SaveValue = std::variant<std::int64_t, double, bool, std::string>;
}
