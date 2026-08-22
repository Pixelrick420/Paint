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
    if (w <= 0 || h <= 0)
        return;

    int sx = wx - bounds.left;
    int sy = wy - bounds.top;
    if (sx < 0 || sx >= w || sy < 0 || sy >= h)
        return;

    int fx0 = bounds.left - originX_;
    int fy0 = bounds.top - originY_;

    auto idx = [&](int x, int y) -> size_t {
        return ((size_t)(fy0 + y) * gridMax + (fx0 + x)) * 4;
    };

    size_t si = idx(sx, sy);
    uint8_t tr = cpu[si], tg = cpu[si + 1], tb = cpu[si + 2];
    if (tr == color.r && tg == color.g && tb == color.b)
        return;

    auto paint = [&](size_t i) {
        cpu[i] = color.r;
        cpu[i + 1] = color.g;
        cpu[i + 2] = color.b;
        cpu[i + 3] = 255;
    };

    std::vector<size_t> stack;
    stack.reserve(w);
    stack.push_back(si);
    paint(si);

    while (!stack.empty())
    {
        size_t pi = stack.back();
        stack.pop_back();

        int px = (int)((pi / 4) % (size_t)gridMax) - fx0;
        int py = (int)((pi / 4) / (size_t)gridMax) - fy0;

        // Fill left.
        int lx = px - 1;
        while (lx >= 0)
        {
            size_t li = idx(lx, py);
            if (cpu[li] == tr && cpu[li + 1] == tg && cpu[li + 2] == tb)
            {
                paint(li);
                --lx;
            }
            else
                break;
        }
        ++lx;

        // Fill right.
        int rx = px + 1;
        while (rx < w)
        {
            size_t ri = idx(rx, py);
            if (cpu[ri] == tr && cpu[ri + 1] == tg && cpu[ri + 2] == tb)
            {
                paint(ri);
                ++rx;
            }
            else
                break;
        }
        --rx;

        // Push matching pixels in the row above.
        if (py > 0)
        {
            for (int x = lx; x <= rx; ++x)
            {
                size_t ni = idx(x, py - 1);
                if (cpu[ni] == tr && cpu[ni + 1] == tg && cpu[ni + 2] == tb)
                {
                    paint(ni);
                    stack.push_back(ni);
                }
            }
        }

        // Push matching pixels in the row below.
        if (py + 1 < h)
        {
            for (int x = lx; x <= rx; ++x)
            {
                size_t ni = idx(x, py + 1);
                if (cpu[ni] == tr && cpu[ni + 1] == tg && cpu[ni + 2] == tb)
                {
                    paint(ni);
                    stack.push_back(ni);
                }
            }
        }
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
