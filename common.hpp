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
