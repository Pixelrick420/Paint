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

    SDL_Rect clip = canvasScreenRect();
    if (clip.w <= 0 || clip.h <= 0)
        return;
    SDL_Rect oldClip;
    SDL_bool hadClip = SDL_RenderIsClipEnabled(renderer);
    if (hadClip)
        SDL_RenderGetClipRect(renderer, &oldClip);
    SDL_RenderSetClipRect(renderer, &clip);

    float viewL = camX, viewT = camY;
    float viewR = camX + (float)SCREEN_WIDTH / zoom;
    float viewB = camY + (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;

    for (int x = floorDiv((int)std::floor(viewL), step) * step; x < (int)std::ceil(viewR); x += step)
    {
        int idx = x / step;
        if (idx % 10 == 0)
            SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);
        else if (idx % 5 == 0)
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
        int sx = (int)std::lround((x - viewL) * zoom);
        SDL_Rect r = {sx, MENU_HEIGHT, 1, SCREEN_HEIGHT - MENU_HEIGHT};
        SDL_RenderFillRect(renderer, &r);
    }
    for (int y = floorDiv((int)std::floor(viewT), step) * step; y < (int)std::ceil(viewB); y += step)
    {
        int idx = y / step;
        if (idx % 10 == 0)
            SDL_SetRenderDrawColor(renderer, 140, 140, 140, 255);
        else if (idx % 5 == 0)
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        else
            SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
        int sy = MENU_HEIGHT + (int)std::lround((y - viewT) * zoom);
        SDL_Rect r = {0, sy, SCREEN_WIDTH, 1};
        SDL_RenderFillRect(renderer, &r);
    }

    SDL_RenderSetClipRect(renderer, hadClip ? &oldClip : nullptr);
}

SDL_Rect PaintApp::canvasScreenRect() const
{
    float viewL = camX, viewT = camY;
    float viewR = camX + (float)SCREEN_WIDTH / zoom;
    float viewB = camY + (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;

    int clipL = std::max(canvas.left(), (int)std::floor(viewL));
    int clipT = std::max(canvas.top(), (int)std::floor(viewT));
    int clipR = std::min(canvas.right(), (int)std::ceil(viewR));
    int clipB = std::min(canvas.bottom(), (int)std::ceil(viewB));

    SDL_Rect r = {0, 0, 0, 0};
    if (clipR > clipL && clipB > clipT)
    {
        r = {
            (int)std::lround((clipL - viewL) * zoom),
            MENU_HEIGHT + (int)std::lround((clipT - viewT) * zoom),
            (int)std::lround((clipR - clipL) * zoom),
            (int)std::lround((clipB - clipT) * zoom)};
    }
    return r;
}

void PaintApp::drawStatusStrip()
{
    SDL_SetRenderDrawColor(renderer, 238, 238, 238, 255);
    SDL_Rect strip = {0, MENU_HEIGHT, SCREEN_WIDTH, STATUS_STRIP_H};
    SDL_RenderFillRect(renderer, &strip);

    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_Rect menuLine = {0, MENU_HEIGHT - 1, SCREEN_WIDTH, 1};
    SDL_RenderFillRect(renderer, &menuLine);

    int eff = effectiveThickness();
    int len = std::clamp(8 + eff * 4, 8, 380);
    int th = 8;
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

    SDL_Rect clip = canvasScreenRect();
    if (clip.w <= 0 || clip.h <= 0)
        return;
    SDL_Rect oldClip;
    SDL_bool hadClip = SDL_RenderIsClipEnabled(renderer);
    if (hadClip)
        SDL_RenderGetClipRect(renderer, &oldClip);
    SDL_RenderSetClipRect(renderer, &clip);

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

    SDL_RenderSetClipRect(renderer, hadClip ? &oldClip : nullptr);
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

void PaintApp::drawScrollBars()
{
    float viewW = (float)SCREEN_WIDTH / zoom;
    float viewH = (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;
    float worldW = (float)canvas.width();
    float worldH = (float)canvas.height();
    float trackLenV = (float)(SCREEN_HEIGHT - MENU_HEIGHT - SCROLLBAR_W);
    float trackLenH = (float)(SCREEN_WIDTH - SCROLLBAR_W);

    SDL_SetRenderDrawColor(renderer, 218, 218, 218, 255);
    SDL_Rect vTrack = {SCREEN_WIDTH - SCROLLBAR_W, MENU_HEIGHT, SCROLLBAR_W, (int)trackLenV};
    SDL_RenderFillRect(renderer, &vTrack);
    SDL_Rect hTrack = {0, SCREEN_HEIGHT - SCROLLBAR_W, (int)trackLenH, SCROLLBAR_W};
    SDL_RenderFillRect(renderer, &hTrack);
    SDL_Rect corner = {SCREEN_WIDTH - SCROLLBAR_W, SCREEN_HEIGHT - SCROLLBAR_W, SCROLLBAR_W, SCROLLBAR_W};
    SDL_RenderFillRect(renderer, &corner);

    if (worldH > viewH)
    {
        float thumbLen = std::max((float)MIN_THUMB_LEN, trackLenV * viewH / worldH);
        float maxScroll = worldH - viewH;
        float frac = (camY - (float)canvas.top()) / maxScroll;
        frac = std::clamp(frac, 0.0f, 1.0f);
        float ty = MENU_HEIGHT + frac * (trackLenV - thumbLen);
        SDL_SetRenderDrawColor(renderer, 158, 158, 158, 255);
        SDL_Rect vThumb = {SCREEN_WIDTH - SCROLLBAR_W, (int)ty, SCROLLBAR_W, (int)thumbLen};
        SDL_RenderFillRect(renderer, &vThumb);
    }

    if (worldW > viewW)
    {
        float thumbLen = std::max((float)MIN_THUMB_LEN, trackLenH * viewW / worldW);
        float maxScroll = worldW - viewW;
        float frac = (camX - (float)canvas.left()) / maxScroll;
        frac = std::clamp(frac, 0.0f, 1.0f);
        float tx = frac * (trackLenH - thumbLen);
        SDL_SetRenderDrawColor(renderer, 158, 158, 158, 255);
        SDL_Rect hThumb = {(int)tx, SCREEN_HEIGHT - SCROLLBAR_W, (int)thumbLen, SCROLLBAR_W};
        SDL_RenderFillRect(renderer, &hThumb);
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
    drawStatusStrip();
    drawPreview();
    drawFlashOverlays();
    drawScrollBars();
    drawHelpPopup();
    SDL_RenderPresent(renderer);
}

}
