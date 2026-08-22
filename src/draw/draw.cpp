#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>

#include "app/app.hpp"

namespace paint
{

void PaintApp::drawPoint(int wx, int wy)
{
    if (previewing)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 160);
        int sx = static_cast<int>(std::lround((wx - camX) * zoom));
        int sy = MENU_HEIGHT + static_cast<int>(std::lround((wy - camY) * zoom));
        SDL_RenderDrawPoint(renderer, sx, sy);
        return;
    }
    if (wx >= canvas.left() && wx < canvas.right() && wy >= canvas.top() && wy < canvas.bottom())
    {
        points.push_back({wx, wy});
        if (points.size() >= POINT_THRESHOLD)
            flushPoints();
    }
}

void PaintApp::drawLine(int x1, int y1, int x2, int y2)
{
    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        for (int tx = -thickness; tx <= thickness; ++tx)
            for (int ty = -thickness; ty <= thickness; ++ty)
                drawPoint(x1 + tx, y1 + ty);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void PaintApp::drawCircle(int cx, int cy, int x, int y)
{
    int radius = static_cast<int>(std::sqrt(static_cast<float>((cx - x) * (cx - x) + (cy - y) * (cy - y))));
    int radiusSq = radius * radius, circleThickness = thickness * 2;
    for (int dx = -radius - circleThickness; dx <= radius + circleThickness; dx++)
        for (int dy = -radius - circleThickness; dy <= radius + circleThickness; dy++)
        {
            int distSq = dx * dx + dy * dy;
            if (std::abs(distSq - radiusSq) <= circleThickness * radius)
                drawPoint(cx + dx, cy + dy);
        }
}

void PaintApp::drawRectangle(int x1, int y1, int x2, int y2)
{
    int left = std::min(x1, x2);
    int right = std::max(x1, x2);
    int top = std::min(y1, y2);
    int bottom = std::max(y1, y2);

    for (int x = left - thickness; x <= right + thickness; x++)
        for (int t = -thickness; t <= thickness; t++)
        {
            drawPoint(x, top + t);
            drawPoint(x, bottom + t);
        }

    for (int y = top - thickness; y <= bottom + thickness; y++)
        for (int t = -thickness; t <= thickness; t++)
        {
            drawPoint(left + t, y);
            drawPoint(right + t, y);
        }
}

void PaintApp::flushPoints()
{
    if (points.empty())
        return;
    for (const SDL_Point &p : points)
        canvas.setGridPixel(p.x - canvas.originX(), p.y - canvas.originY(), color);
    points.clear();
    dirty = true;
}

void PaintApp::ensureCanvasCoversView()
{
    const ViewRect view = viewRect();

    int needL = floorDiv(static_cast<int>(std::floor(view.l)), CHUNK) * CHUNK;
    int needT = floorDiv(static_cast<int>(std::floor(view.t)), CHUNK) * CHUNK;
    int needR = ceilDiv(static_cast<int>(std::ceil(view.r)), CHUNK) * CHUNK;
    int needB = ceilDiv(static_cast<int>(std::ceil(view.b)), CHUNK) * CHUNK;

    if (needL < canvas.left() && canvas.right() - needL <= canvas.gridSize())
        canvas.expandLeft(canvas.left() - needL);
    if (needR > canvas.right() && needR - canvas.left() <= canvas.gridSize())
        canvas.expandRight(needR - canvas.right());
    if (needT < canvas.top() && canvas.bottom() - needT <= canvas.gridSize())
        canvas.expandTop(canvas.top() - needT);
    if (needB > canvas.bottom() && needB - canvas.top() <= canvas.gridSize())
        canvas.expandBottom(needB - canvas.bottom());
}

void PaintApp::saveCanvasBMP()
{
    int w = canvas.width();
    int h = canvas.height();
    if (w <= 0 || h <= 0)
        return;

    canvas.copyBoundsTo(fillBuf);

    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char name[64];
    std::snprintf(name, sizeof(name), "paint_%04d%02d%02d_%02d%02d%02d.bmp",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);

    if (!writeBMPFile(name, fillBuf.data(), w, h))
    {
        std::cerr << "Save failed: " << name << std::endl;
        return;
    }

    std::cout << "Saved " << name << " (" << w << "x" << h << ")" << std::endl;
    saveFlashUntil = SDL_GetTicks() + SAVE_FLASH_MS;
}

}
