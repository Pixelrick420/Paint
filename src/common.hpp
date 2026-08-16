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
