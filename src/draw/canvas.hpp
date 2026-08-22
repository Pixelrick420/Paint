#pragma once

#include <cstdint>
#include <vector>

#include "common.hpp"

namespace paint
{

struct WorldRect
{
    int left, top, right, bottom;
};

// Canvas pixel store. No SDL dependency. Stores RGBA bytes in a
// gridMax^2 buffer; bounds and origin map world coordinates to it.
class Canvas
{
public:
    void init(int gridMax, int initialW, int initialH);

    int gridSize() const { return gridMax; }
    int width() const { return bounds.right - bounds.left; }
    int height() const { return bounds.bottom - bounds.top; }
    int left() const { return bounds.left; }
    int top() const { return bounds.top; }
    int right() const { return bounds.right; }
    int bottom() const { return bounds.bottom; }
    int originX() const { return originX_; }
    int originY() const { return originY_; }

    uint8_t *pixelData() { return cpu.data(); }
    const uint8_t *pixelData() const { return cpu.data(); }

    void clear();

    void expandLeft(int n);
    void expandRight(int n);
    void expandTop(int m);
    void expandBottom(int m);

    void setGridPixel(int gx, int gy, Color c);

    void floodFill(int wx, int wy, Color color);

    // Copies the bounds region into dst as top-down RGBA.
    void copyBoundsTo(std::vector<uint8_t> &dst) const;

private:
    int gridMax = 0;
    int originX_ = 0;
    int originY_ = 0;
    WorldRect bounds{0, 0, 0, 0};
    int initialW = 0;
    int initialH = 0;
    std::vector<uint8_t> cpu;
};

}
