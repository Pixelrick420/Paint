#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>

#include "app.hpp"

namespace paint
{

void PaintApp::drawCanvasView()
{
    SDL_SetRenderDrawColor(renderer, 205, 205, 205, 255);
    SDL_Rect area = {0, MENU_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - MENU_HEIGHT};
    SDL_RenderFillRect(renderer, &area);

    float viewL = camX;
    float viewT = camY;
    float viewR = camX + (float)SCREEN_WIDTH / zoom;
    float viewB = camY + (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;

    int clipL = std::max(canvas.left(), (int)std::floor(viewL));
    int clipT = std::max(canvas.top(), (int)std::floor(viewT));
    int clipR = std::min(canvas.right(), (int)std::ceil(viewR));
    int clipB = std::min(canvas.bottom(), (int)std::ceil(viewB));

    if (clipR > clipL && clipB > clipT)
    {
        SDL_Rect texRect = {clipL - canvas.originX(), clipT - canvas.originY(), clipR - clipL, clipB - clipT};
        SDL_Rect dst = {
            (int)std::lround((clipL - viewL) * zoom),
            MENU_HEIGHT + (int)std::lround((clipT - viewT) * zoom),
            (int)std::lround((clipR - clipL) * zoom),
            (int)std::lround((clipB - clipT) * zoom)};

        bool clipChanged = lastClip.x != texRect.x || lastClip.y != texRect.y ||
                           lastClip.w != texRect.w || lastClip.h != texRect.h;
        lastClip = texRect;

        if (dirty || clipChanged)
        {
            int cw = texRect.w, ch = texRect.h;
            const uint8_t *src = &canvas.pixelData()[((size_t)texRect.y * canvas.gridSize() + texRect.x) * 4];
            uploadBuf.resize((size_t)cw * ch * 4);
            for (int r = 0; r < ch; ++r)
            {
                const uint8_t *row = src + (size_t)r * canvas.gridSize() * 4;
                uint8_t *dstRow = &uploadBuf[(size_t)r * cw * 4];
                for (int c = 0; c < cw; ++c)
                {
                    dstRow[c * 4 + 0] = row[c * 4 + 3];
                    dstRow[c * 4 + 1] = row[c * 4 + 2];
                    dstRow[c * 4 + 2] = row[c * 4 + 1];
                    dstRow[c * 4 + 3] = row[c * 4 + 0];
                }
            }
            SDL_UpdateTexture(canvasTex, &texRect, uploadBuf.data(), cw * 4);
            dirty = false;
        }

        if (dst.w > 0 && dst.h > 0)
            SDL_RenderCopy(renderer, canvasTex, &texRect, &dst);
    }

    if (showGrid)
        drawGrid();
}

void PaintApp::drawGrid()
{
    int step = 1;
    while ((float)step * zoom < MIN_GRID_PX)
        step *= 2;

    float viewL = camX, viewT = camY;
    float viewR = camX + (float)SCREEN_WIDTH / zoom;
    float viewB = camY + (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;

    SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
    for (int x = floorDiv((int)std::floor(viewL), step) * step; x < (int)std::ceil(viewR); x += step)
    {
        int sx = (int)std::lround((x - viewL) * zoom);
        SDL_RenderDrawLine(renderer, sx, MENU_HEIGHT, sx, SCREEN_HEIGHT);
    }
    for (int y = floorDiv((int)std::floor(viewT), step) * step; y < (int)std::ceil(viewB); y += step)
    {
        int sy = MENU_HEIGHT + (int)std::lround((y - viewT) * zoom);
        SDL_RenderDrawLine(renderer, 0, sy, SCREEN_WIDTH, sy);
    }
}

void PaintApp::drawFillIcon()
{
    if (!fillTex)
        return;
    int slotX = 7 * TOOL_WIDTH;
    SDL_Rect dst = {slotX + (TOOL_WIDTH - fillTexW) / 2, (ROW_HEIGHT - fillTexH) / 2, fillTexW, fillTexH};
    SDL_RenderCopy(renderer, fillTex, nullptr, &dst);
}

void PaintApp::drawStatusStrip()
{
    SDL_SetRenderDrawColor(renderer, 238, 238, 238, 255);
    SDL_Rect strip = {0, MENU_HEIGHT, SCREEN_WIDTH, STATUS_STRIP_H};
    SDL_RenderFillRect(renderer, &strip);

    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_Rect menuLine = {0, MENU_HEIGHT - 1, SCREEN_WIDTH, 1};
    SDL_RenderFillRect(renderer, &menuLine);

    if (SDL_GetTicks() < thickFlashUntil)
    {
        SDL_SetRenderDrawColor(renderer, 255, 235, 130, 255);
        SDL_Rect fl = {6, MENU_HEIGHT + 5, 8 + 380 + 8, STATUS_STRIP_H - 10};
        SDL_RenderFillRect(renderer, &fl);
    }

    int eff = effectiveThickness();
    int len = std::clamp(8 + eff * 4, 8, 380);
    int th = 2;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_Rect bar = {12, MENU_HEIGHT + STATUS_STRIP_H / 2 - th / 2, len, th};
    SDL_RenderFillRect(renderer, &bar);

    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_Rect sep = {0, MENU_HEIGHT + STATUS_STRIP_H - 2, SCREEN_WIDTH, 2};
    SDL_RenderFillRect(renderer, &sep);
}

void PaintApp::drawPreview()
{
    if (shapeStart.x == -1 || tool < 3 || tool > 5)
        return;
    int wx = toWorldX(lastMouseX), wy = toWorldY(lastMouseY);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    previewing = true;
    switch (tool)
    {
    case 3:
        drawLine(shapeStart.x, shapeStart.y, wx, wy);
        break;
    case 4:
        drawCircle(shapeStart.x, shapeStart.y, wx, wy);
        break;
    case 5:
        drawRectangle(shapeStart.x, shapeStart.y, wx, wy);
        break;
    }
    previewing = false;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PaintApp::drawFlashOverlays()
{
    if (SDL_GetTicks() < saveFlashUntil)
    {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 60, 200, 90, 130);
        SDL_Rect r = {SCREEN_WIDTH - 96, MENU_HEIGHT + 14, 84, 22};
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

void PaintApp::drawScreen()
{
    flushPoints();
    SDL_SetRenderDrawColor(renderer, 205, 205, 205, 255);
    SDL_RenderClear(renderer);
    drawCanvasView();
    if (menuTex)
    {
        SDL_Rect menuDst = {0, 0, SCREEN_WIDTH, MENU_HEIGHT};
        SDL_RenderCopy(renderer, menuTex, nullptr, &menuDst);
    }
    drawFillIcon();
    drawStatusStrip();
    drawPreview();
    drawFlashOverlays();
    SDL_RenderPresent(renderer);
}

}
