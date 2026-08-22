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

// Authoritative pixel store for the paint canvas. Pure data structure with
// no SDL dependency: every pixel lives here as RGBA bytes in a gridMax^2
// buffer, and bounds/origin map world coordinates onto that buffer.
class Canvas
{
public:
    void init(int gridMax, int initialW, int initialH);

    // Accessors -------------------------------------------------------------
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

    // Operations ------------------------------------------------------------
    void clear();

    void expandLeft(int n);
    void expandRight(int n);
    void expandTop(int m);
    void expandBottom(int m);

    void setGridPixel(int gx, int gy, Color c);

    void floodFill(int wx, int wy, Color color);

    // Copies the current bounds region (RGBA, top-down) into dst.
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
