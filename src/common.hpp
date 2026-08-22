#pragma once

#include <array>
#include <string>
#include <unistd.h>

#include "constants.hpp"

namespace paint
{

enum class Tool
{
    Pencil = 1,
    Eraser,
    Line,
    Circle,
    Rectangle,
    Fill,
};

// A single tool slot in the menu. Row/col position is shared by the icon
// compositor (buildMenuTexture) and the click hit-testing (handleMenuClick),
// so the layout lives in exactly one place.
struct ToolSlot
{
    int row, col;
    Tool tool;
    const char *icon;    // BMP asset name under assets/
    bool forceColor;     // selecting this tool also forces the drawing color
    int forceColorIndex; // palette index forced when forceColor is true
};

inline constexpr int NUM_TOOLS = 6;
inline constexpr std::array<ToolSlot, NUM_TOOLS> toolSlots = {{
    {0, 0, Tool::Pencil, "tool_pencil.bmp", true, 0},
    {0, 1, Tool::Line, "tool_line.bmp", false, 0},
    {0, 2, Tool::Rectangle, "tool_rectangle.bmp", false, 0},
    {1, 0, Tool::Eraser, "tool_eraser.bmp", true, 7},
    {1, 1, Tool::Circle, "tool_circle.bmp", false, 0},
    {1, 2, Tool::Fill, "tool_fill.bmp", false, 0}, // former help slot
}};

inline constexpr std::array<Color, NUM_COLORS> colors = {{
    {0, 0, 0},       // black
    {163, 73, 164},  // purple
    {63, 72, 204},   // blue
    {34, 177, 76},   // green
    {255, 201, 14},  // yellow
    {237, 28, 36},   // red
    {127, 127, 127}, // gray
    {255, 255, 255}  // white
}};

// Color order in the menu: top row even indices, bottom row odd indices.
inline constexpr std::array<int, NUM_COLORS> menuColorOrder = {{0, 2, 4, 6, 1, 3, 5, 7}};

// Euclidean division with floor semantics (works for negative operands).
[[nodiscard]] constexpr int floorDiv(int a, int b)
{
    int q = a / b;
    if (a % b != 0 && ((a < 0) != (b < 0)))
        --q;
    return q;
}

[[nodiscard]] constexpr int ceilDiv(int a, int b)
{
    return -floorDiv(-a, b);
}

[[nodiscard]] inline std::string assetPath(const std::string &name)
{
    std::string base = "assets";
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0)
    {
        buf[len] = '\0';
        std::string exePath(buf);
        std::string::size_type slash = exePath.find_last_of('/');
        if (slash != std::string::npos)
            base = exePath.substr(0, slash) + "/assets";
    }
    return base + "/" + name;
}

}
