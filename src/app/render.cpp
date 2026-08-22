#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>

#include "app.hpp"

namespace paint
{

namespace
{

// Saves the renderer's clip rect (if any), applies a new one for the object's
// lifetime, and restores it on scope exit.
class ScopedClip
{
public:
    ScopedClip(SDL_Renderer *ren, const SDL_Rect &rect) : renderer(ren)
    {
        hadClip = SDL_RenderIsClipEnabled(ren);
        if (hadClip)
            SDL_RenderGetClipRect(ren, &previous);
        SDL_RenderSetClipRect(ren, &rect);
    }

    ~ScopedClip() { SDL_RenderSetClipRect(renderer, hadClip ? &previous : nullptr); }

    ScopedClip(const ScopedClip &) = delete;
    ScopedClip &operator=(const ScopedClip &) = delete;

private:
    SDL_Renderer *renderer;
    SDL_Rect previous{0, 0, 0, 0};
    bool hadClip = false;
};

// Grid line shade by cell index tier (every 10th cell darkest).
SDL_Color gridLineColor(int idx)
{
    if (idx % 10 == 0)
        return {GRID_LINE_10X.r, GRID_LINE_10X.g, GRID_LINE_10X.b, 255};
    if (idx % 5 == 0)
        return {GRID_LINE_5X.r, GRID_LINE_5X.g, GRID_LINE_5X.b, 255};
    return {GRID_LINE.r, GRID_LINE.g, GRID_LINE.b, 255};
}

} // namespace

void PaintApp::drawCanvasView()
{
    setDrawColor(CANVAS_BG);
    SDL_Rect area = {0, MENU_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - MENU_HEIGHT};
    SDL_RenderFillRect(renderer, &area);

    const ViewRect view = viewRect();

    int clipL = std::max(canvas.left(), static_cast<int>(std::floor(view.l)));
    int clipT = std::max(canvas.top(), static_cast<int>(std::floor(view.t)));
    int clipR = std::min(canvas.right(), static_cast<int>(std::ceil(view.r)));
    int clipB = std::min(canvas.bottom(), static_cast<int>(std::ceil(view.b)));

    if (clipR > clipL && clipB > clipT)
    {
        SDL_Rect texRect = {clipL - canvas.originX(), clipT - canvas.originY(),
                            clipR - clipL, clipB - clipT};
        SDL_Rect dst = {
            static_cast<int>(std::lround((clipL - view.l) * zoom)),
            MENU_HEIGHT + static_cast<int>(std::lround((clipT - view.t) * zoom)),
            static_cast<int>(std::lround((clipR - clipL) * zoom)),
            static_cast<int>(std::lround((clipB - clipT) * zoom))};

        bool clipChanged = lastClip.x != texRect.x || lastClip.y != texRect.y ||
                           lastClip.w != texRect.w || lastClip.h != texRect.h;
        lastClip = texRect;

        if (dirty || clipChanged)
        {
            int cw = texRect.w, ch = texRect.h;
            const uint8_t *src =
                &canvas.pixelData()[(static_cast<size_t>(texRect.y) * canvas.gridSize() +
                                     texRect.x) *
                                    4];
            uploadBuf.resize(static_cast<size_t>(cw) * ch * 4);
            for (int r = 0; r < ch; ++r)
            {
                const uint8_t *row = src + static_cast<size_t>(r) * canvas.gridSize() * 4;
                uint8_t *dstRow = &uploadBuf[static_cast<size_t>(r) * cw * 4];
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
    while (static_cast<float>(step) * zoom < MIN_GRID_PX)
        step *= 2;

    SDL_Rect clip = canvasScreenRect();
    if (clip.w <= 0 || clip.h <= 0)
        return;
    ScopedClip scoped(renderer, clip);

    const ViewRect view = viewRect();

    for (int x = floorDiv(static_cast<int>(std::floor(view.l)), step) * step;
         x < static_cast<int>(std::ceil(view.r)); x += step)
    {
        SDL_Color c = gridLineColor(x / step);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        int sx = static_cast<int>(std::lround((x - view.l) * zoom));
        SDL_Rect r = {sx, MENU_HEIGHT, 1, SCREEN_HEIGHT - MENU_HEIGHT};
        SDL_RenderFillRect(renderer, &r);
    }
    for (int y = floorDiv(static_cast<int>(std::floor(view.t)), step) * step;
         y < static_cast<int>(std::ceil(view.b)); y += step)
    {
        SDL_Color c = gridLineColor(y / step);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        int sy = MENU_HEIGHT + static_cast<int>(std::lround((y - view.t) * zoom));
        SDL_Rect r = {0, sy, SCREEN_WIDTH, 1};
        SDL_RenderFillRect(renderer, &r);
    }
}

SDL_Rect PaintApp::canvasScreenRect() const
{
    const ViewRect view = viewRect();

    int clipL = std::max(canvas.left(), static_cast<int>(std::floor(view.l)));
    int clipT = std::max(canvas.top(), static_cast<int>(std::floor(view.t)));
    int clipR = std::min(canvas.right(), static_cast<int>(std::ceil(view.r)));
    int clipB = std::min(canvas.bottom(), static_cast<int>(std::ceil(view.b)));

    SDL_Rect r = {0, 0, 0, 0};
    if (clipR > clipL && clipB > clipT)
    {
        r = {
            static_cast<int>(std::lround((clipL - view.l) * zoom)),
            MENU_HEIGHT + static_cast<int>(std::lround((clipT - view.t) * zoom)),
            static_cast<int>(std::lround((clipR - clipL) * zoom)),
            static_cast<int>(std::lround((clipB - clipT) * zoom))};
    }
    return r;
}

void PaintApp::setDrawColor(uint8_t grey)
{
    SDL_SetRenderDrawColor(renderer, grey, grey, grey, 255);
}

void PaintApp::drawStatusStrip()
{
    setDrawColor(STATUS_BG.r);
    SDL_Rect strip = {0, MENU_HEIGHT, SCREEN_WIDTH, STATUS_STRIP_H};
    SDL_RenderFillRect(renderer, &strip);

    setDrawColor(STATUS_EDGE.r);
    SDL_Rect menuLine = {0, MENU_HEIGHT - 1, SCREEN_WIDTH, 1};
    SDL_RenderFillRect(renderer, &menuLine);

    int eff = effectiveThickness();
    int len = std::clamp(THICKNESS_BAR_MIN_LEN + eff * THICKNESS_BAR_SCALE,
                         THICKNESS_BAR_MIN_LEN, THICKNESS_BAR_MAX_LEN);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_Rect bar = {THICKNESS_BAR_X, MENU_HEIGHT + STATUS_STRIP_H / 2 - THICKNESS_BAR_H / 2,
                    len, THICKNESS_BAR_H};
    SDL_RenderFillRect(renderer, &bar);

    setDrawColor(STATUS_SEP.r);
    SDL_Rect sep = {0, MENU_HEIGHT + STATUS_STRIP_H - 2, SCREEN_WIDTH, 2};
    SDL_RenderFillRect(renderer, &sep);
}

void PaintApp::drawPreview()
{
    bool shapeTool = tool >= Tool::Line && tool <= Tool::Rectangle;
    if (shapeStart.x == -1 || !shapeTool)
        return;

    SDL_Rect clip = canvasScreenRect();
    if (clip.w <= 0 || clip.h <= 0)
        return;
    ScopedClip scoped(renderer, clip);

    int wx = toWorldX(lastMouseX), wy = toWorldY(lastMouseY);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    previewing = true;
    drawShape(shapeStart.x, shapeStart.y, wx, wy);
    previewing = false;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PaintApp::drawFlashOverlays()
{
    if (SDL_GetTicks() >= saveFlashUntil)
        return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, FLASH_BG.r, FLASH_BG.g, FLASH_BG.b, FLASH_ALPHA);
    SDL_Rect r = {SCREEN_WIDTH - SAVE_FLASH_MARGIN_R, MENU_HEIGHT + SAVE_FLASH_MARGIN_T,
                  SAVE_FLASH_W, SAVE_FLASH_H};
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void PaintApp::drawScrollBars()
{
    float trackLenV = verticalTrackLen();
    float trackLenH = horizontalTrackLen();

    setDrawColor(SCROLL_TRACK.r);
    SDL_Rect vTrack = {SCREEN_WIDTH - SCROLLBAR_W, MENU_HEIGHT, SCROLLBAR_W,
                       static_cast<int>(trackLenV)};
    SDL_RenderFillRect(renderer, &vTrack);
    SDL_Rect hTrack = {0, SCREEN_HEIGHT - SCROLLBAR_W, static_cast<int>(trackLenH), SCROLLBAR_W};
    SDL_RenderFillRect(renderer, &hTrack);
    SDL_Rect corner = {SCREEN_WIDTH - SCROLLBAR_W, SCREEN_HEIGHT - SCROLLBAR_W,
                       SCROLLBAR_W, SCROLLBAR_W};
    SDL_RenderFillRect(renderer, &corner);

    setDrawColor(SCROLL_THUMB.r);
    if (canvas.height() > viewHeight())
    {
        ScrollMetrics m = scrollMetrics(static_cast<float>(canvas.height()), viewHeight(),
                                        trackLenV);
        float frac = std::clamp((camY - static_cast<float>(canvas.top())) / m.maxScroll, 0.0f, 1.0f);
        float ty = MENU_HEIGHT + frac * (trackLenV - m.thumbLen);
        SDL_Rect vThumb = {SCREEN_WIDTH - SCROLLBAR_W, static_cast<int>(ty),
                           SCROLLBAR_W, static_cast<int>(m.thumbLen)};
        SDL_RenderFillRect(renderer, &vThumb);
    }

    if (canvas.width() > viewWidth())
    {
        ScrollMetrics m = scrollMetrics(static_cast<float>(canvas.width()), viewWidth(),
                                        trackLenH);
        float frac = std::clamp((camX - static_cast<float>(canvas.left())) / m.maxScroll, 0.0f, 1.0f);
        float tx = frac * (trackLenH - m.thumbLen);
        SDL_Rect hThumb = {static_cast<int>(tx), SCREEN_HEIGHT - SCROLLBAR_W,
                           static_cast<int>(m.thumbLen), SCROLLBAR_W};
        SDL_RenderFillRect(renderer, &hThumb);
    }
}

void PaintApp::drawShape(int x1, int y1, int x2, int y2)
{
    switch (tool)
    {
    case Tool::Line:
        drawLine(x1, y1, x2, y2);
        break;
    case Tool::Circle:
        drawCircle(x1, y1, x2, y2);
        break;
    case Tool::Rectangle:
        drawRectangle(x1, y1, x2, y2);
        break;
    default:
        break;
    }
}

void PaintApp::drawScreen()
{
    flushPoints();
    setDrawColor(CANVAS_BG);
    SDL_RenderClear(renderer);
    drawCanvasView();
    if (menuTex != nullptr)
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
