#include <algorithm>
#include <cstring>
#include <vector>

#include "canvas.hpp"

namespace paint
{

void Canvas::init(int gridMax_, int initialW_, int initialH_)
{
    gridMax = gridMax_;
    initialW = initialW_;
    initialH = initialH_;
    cpu.assign((size_t)gridMax * gridMax * 4, 255);
    originX_ = 0;
    originY_ = 0;
    bounds = {0, 0, std::min(initialW, gridMax), std::min(initialH, gridMax)};
}

void Canvas::clear()
{
    std::fill(cpu.begin(), cpu.end(), 255);
    originX_ = 0;
    originY_ = 0;
    bounds = {0, 0, std::min(initialW, gridMax), std::min(initialH, gridMax)};
}

void Canvas::setGridPixel(int gx, int gy, Color c)
{
    if (gx < 0 || gy < 0 || gx >= gridMax || gy >= gridMax)
        return;
    size_t i = ((size_t)gy * gridMax + gx) * 4;
    cpu[i] = c.r;
    cpu[i + 1] = c.g;
    cpu[i + 2] = c.b;
    cpu[i + 3] = 255;
}

void Canvas::expandLeft(int n)
{
    if (n <= 0)
        return;
    for (int y = gridMax - 1; y >= 0; --y)
    {
        uint8_t *row = &cpu[(size_t)y * gridMax * 4];
        std::memmove(row + (size_t)n * 4, row, (size_t)(gridMax - n) * 4);
        std::fill(row, row + (size_t)n * 4, 255);
    }
    originX_ -= n;
    bounds.left -= n;
}

void Canvas::expandRight(int n)
{
    if (n <= 0)
        return;
    int h = bounds.bottom - bounds.top;
    int tx0 = bounds.right - originX_;
    int ty0 = bounds.top - originY_;
    for (int y = 0; y < h; ++y)
    {
        uint8_t *row = &cpu[((size_t)(ty0 + y) * gridMax + tx0) * 4];
        std::fill(row, row + (size_t)n * 4, 255);
    }
    bounds.right += n;
}

void Canvas::expandTop(int m)
{
    if (m <= 0)
        return;
    for (int y = gridMax - 1; y >= m; --y)
    {
        std::memcpy(&cpu[(size_t)y * gridMax * 4], &cpu[(size_t)(y - m) * gridMax * 4],
                    (size_t)gridMax * 4);
    }
    std::fill(cpu.begin(), cpu.begin() + (size_t)m * gridMax * 4, 255);
    originY_ -= m;
    bounds.top -= m;
}

void Canvas::expandBottom(int m)
{
    if (m <= 0)
        return;
    int w = bounds.right - bounds.left;
    int ty0 = bounds.bottom - originY_;
    for (int y = 0; y < m; ++y)
    {
        uint8_t *row = &cpu[((size_t)(ty0 + y) * gridMax + (bounds.left - originX_)) * 4];
        std::fill(row, row + (size_t)w * 4, 255);
    }
    bounds.bottom += m;
}

void Canvas::floodFill(int wx, int wy, Color color)
{
    if (wx < bounds.left || wx >= bounds.right || wy < bounds.top || wy >= bounds.bottom)
        return;

    int w = bounds.right - bounds.left;
    int h = bounds.bottom - bounds.top;
    size_t n = (size_t)w * h;

    std::vector<uint8_t> buf(n * 4);
    uint8_t *px = buf.data();
    int fx0 = bounds.left - originX_;
    int fy0 = bounds.top - originY_;
    for (int y = 0; y < h; ++y)
    {
        const uint8_t *src = &cpu[((size_t)(fy0 + y) * gridMax + fx0) * 4];
        std::memcpy(px + (size_t)y * w * 4, src, (size_t)w * 4);
    }

    int sx = wx - bounds.left;
    int sy = wy - bounds.top;
    if (sx < 0 || sx >= w || sy < 0 || sy >= h)
        return;

    size_t seed = (size_t)sy * w + sx;
    uint8_t tr = px[seed * 4], tg = px[seed * 4 + 1], tb = px[seed * 4 + 2];
    if (tr == color.r && tg == color.g && tb == color.b)
        return;

    std::vector<uint8_t> visited(n, 0);
    std::vector<size_t> stack;
    stack.reserve(n);
    stack.push_back(seed);
    visited[seed] = 1;

    while (!stack.empty())
    {
        size_t idx = stack.back();
        stack.pop_back();
        size_t pi = idx * 4;
        if (px[pi] == tr && px[pi + 1] == tg && px[pi + 2] == tb)
        {
            px[pi] = color.r;
            px[pi + 1] = color.g;
            px[pi + 2] = color.b;
            px[pi + 3] = 255;

            int x = (int)(idx % w);
            int y = (int)(idx / w);
            if (x > 0)
            {
                size_t ni = idx - 1;
                if (!visited[ni]) { visited[ni] = 1; stack.push_back(ni); }
            }
            if (x + 1 < w)
            {
                size_t ni = idx + 1;
                if (!visited[ni]) { visited[ni] = 1; stack.push_back(ni); }
            }
            if (y > 0)
            {
                size_t ni = idx - w;
                if (!visited[ni]) { visited[ni] = 1; stack.push_back(ni); }
            }
            if (y + 1 < h)
            {
                size_t ni = idx + w;
                if (!visited[ni]) { visited[ni] = 1; stack.push_back(ni); }
            }
        }
    }

    for (int y = 0; y < h; ++y)
    {
        uint8_t *dst = &cpu[((size_t)(fy0 + y) * gridMax + fx0) * 4];
        std::memcpy(dst, px + (size_t)y * w * 4, (size_t)w * 4);
    }
}

void Canvas::copyBoundsTo(std::vector<uint8_t> &dst) const
{
    int w = bounds.right - bounds.left;
    int h = bounds.bottom - bounds.top;
    if (w <= 0 || h <= 0)
        return;
    if (dst.size() < (size_t)w * h * 4)
        dst.resize((size_t)w * h * 4);
    int fx0 = bounds.left - originX_;
    int fy0 = bounds.top - originY_;
    for (int y = 0; y < h; ++y)
    {
        const uint8_t *src = &cpu[((size_t)(fy0 + y) * gridMax + fx0) * 4];
        std::memcpy(dst.data() + (size_t)y * w * 4, src, (size_t)w * 4);
    }
}

}
