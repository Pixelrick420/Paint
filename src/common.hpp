#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unistd.h>

namespace paint
{

inline constexpr int NUM_COLORS = 8;
inline constexpr int SCREEN_WIDTH = 1280;
inline constexpr int SCREEN_HEIGHT = 720;
inline constexpr int POINT_THRESHOLD = 100000;
inline constexpr int MENU_HEIGHT = 112;
inline constexpr int TOOL_WIDTH = 50;
inline constexpr int ROW_HEIGHT = 56;

// Menu layout: tools on the left, colors on the right.
inline constexpr int MENU_COLORS_PER_ROW = NUM_COLORS / 2;
inline constexpr int MENU_COLOR_LEFT =
    SCREEN_WIDTH - MENU_COLORS_PER_ROW * TOOL_WIDTH; // right-aligned block

// A single tool slot in the menu. Row/col position is shared by the icon
// compositor (buildMenuTexture) and the click hit-testing (handleMenuClick),
// so the layout lives in exactly one place.
struct ToolSlot
{
    int row, col;
    int tool;            // 1=pencil, 2=eraser, 3=line, 4=circle, 5=rectangle, 6=fill
    const char *icon;    // BMP asset name under assets/
    bool forceColor;     // selecting this tool also forces the drawing color
    int forceColorIndex; // palette index forced when forceColor is true
};

inline constexpr int NUM_TOOLS = 6;
inline constexpr std::array<ToolSlot, NUM_TOOLS> toolSlots = {{
    {0, 0, 1, "tool_pencil.bmp", true, 0},     // pencil
    {0, 1, 3, "tool_line.bmp", false, 0},      // line
    {0, 2, 5, "tool_rectangle.bmp", false, 0}, // rectangle
    {1, 0, 2, "tool_eraser.bmp", true, 7},     // eraser
    {1, 1, 4, "tool_circle.bmp", false, 0},    // circle
    {1, 2, 6, "tool_fill.bmp", false, 0},      // fill (former help slot)
}};

// Growing-canvas settings
inline constexpr int MAX_GRID = 4096;                 // hard cap: max cells per side
inline constexpr int INITIAL_CANVAS_WIDTH = SCREEN_WIDTH;
inline constexpr int INITIAL_CANVAS_HEIGHT = SCREEN_HEIGHT - MENU_HEIGHT;
inline constexpr int CHUNK = 64;                      // canvas growth step (cells)
inline constexpr float MIN_ZOOM = 0.1f;
inline constexpr float MAX_ZOOM = 32.0f;
inline constexpr int MIN_GRID_PX = 10;                // min on-screen grid spacing
inline constexpr int STATUS_STRIP_H = 20;             // strip between menu and canvas
inline constexpr int MAX_LINE_THICKNESS = 64;
inline constexpr int SCROLLBAR_W = 12;                // scroll bar thickness (px)
inline constexpr int MIN_THUMB_LEN = 24;              // minimum scroll bar thumb length
inline constexpr float SCROLL_PAN = 60.0f;            // world pan per wheel notch

struct Color
{
    uint8_t r, g, b;
};

struct Slider
{
    int x, y, value;
};

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

inline std::string assetPath(const std::string &name)
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
