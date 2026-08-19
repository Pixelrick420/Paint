#include <SDL2/SDL.h>

#include <algorithm>
#include <string>

#include "app/app.hpp"

namespace paint
{

void PaintApp::zoomAt(int mx, int my, float factor)
{
    float oldZoom = zoom;
    float wxm = camX + mx / oldZoom;
    float wym = camY + (my - MENU_HEIGHT) / oldZoom;
    zoom = std::clamp(zoom * factor, MIN_ZOOM, MAX_ZOOM);
    if (zoom == oldZoom)
        return;
    camX = wxm - mx / zoom;
    camY = wym - (my - MENU_HEIGHT) / zoom;
    clampCamera();
    dirty = true;
    setCursorForTool();
}

void PaintApp::beginScrollDrag(int axis, int mx, int my)
{
    float viewW = (float)SCREEN_WIDTH / zoom;
    float viewH = (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;
    float trackLenV = (float)(SCREEN_HEIGHT - MENU_HEIGHT - SCROLLBAR_W);
    float trackLenH = (float)(SCREEN_WIDTH - SCROLLBAR_W);

    if (axis == 1)
    {
        float worldH = (float)canvas.height();
        if (worldH <= viewH)
        {
            scrollDrag = 0;
            return;
        }
        float thumbLen = std::max((float)MIN_THUMB_LEN, trackLenV * viewH / worldH);
        float maxScroll = worldH - viewH;
        float frac = (my - MENU_HEIGHT - thumbLen / 2.0f) / (trackLenV - thumbLen);
        frac = std::clamp(frac, 0.0f, 1.0f);
        camY = (float)canvas.top() + frac * maxScroll;
        scrollDrag = 1;
        scrollDragStartY = my;
        scrollDragStartCamY = camY;
    }
    else
    {
        float worldW = (float)canvas.width();
        if (worldW <= viewW)
        {
            scrollDrag = 0;
            return;
        }
        float thumbLen = std::max((float)MIN_THUMB_LEN, trackLenH * viewW / worldW);
        float maxScroll = worldW - viewW;
        float frac = (mx - thumbLen / 2.0f) / (trackLenH - thumbLen);
        frac = std::clamp(frac, 0.0f, 1.0f);
        camX = (float)canvas.left() + frac * maxScroll;
        scrollDrag = 2;
        scrollDragStartX = mx;
        scrollDragStartCamX = camX;
    }
    dirty = true;
}

void PaintApp::handleKey(const SDL_KeyboardEvent &key)
{
    if (key.keysym.mod & KMOD_CTRL)
    {
        switch (key.keysym.sym)
        {
        case SDLK_s:
            saveCanvasBMP();
            break;
        case SDLK_g:
            showGrid = !showGrid;
            break;
        case SDLK_SLASH:
        case SDLK_KP_DIVIDE:
            toggleHelp();
            break;
        default:
            break;
        }
        return;
    }

    if (key.keysym.sym >= SDLK_1 && key.keysym.sym <= SDLK_7)
        color = colors[key.keysym.sym - SDLK_1];

    switch (key.keysym.sym)
    {
    case SDLK_c:
        clearScreen();
        break;
    case SDLK_z:
        thickness = std::clamp(thickness + 1, 1, MAX_LINE_THICKNESS);
        thickFlashUntil = SDL_GetTicks() + 600;
        setCursorForTool();
        break;
    case SDLK_x:
        thickness = std::clamp(thickness - 1, 1, MAX_LINE_THICKNESS);
        thickFlashUntil = SDL_GetTicks() + 600;
        setCursorForTool();
        break;
    case SDLK_f:
        tool = 6;
        setCursorForTool();
        break;
    case SDLK_ESCAPE:
        if (helpOpen)
            closeHelp();
        break;
    case SDLK_LEFT:
        camX -= 40.0f / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_RIGHT:
        camX += 40.0f / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_UP:
        camY -= 40.0f / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_DOWN:
        camY += 40.0f / zoom;
        clampCamera();
        dirty = true;
        break;
    default:
        break;
    }
}

void PaintApp::handleInput()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_QUIT:
            quit = true;
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch (e.button.button)
            {
            case SDL_BUTTON_LEFT:
            {
                int mx = e.button.x, my = e.button.y;
                lastMouseX = mx;
                lastMouseY = my;
                if (helpOpen)
                {
                    // Clicks while the help popup is open either close it
                    // (close button or outside the panel) or are swallowed.
                    if (mx >= helpClose.x && mx < helpClose.x + helpClose.w &&
                        my >= helpClose.y && my < helpClose.y + helpClose.h)
                        closeHelp();
                    else if (mx < helpPanel.x || mx >= helpPanel.x + helpPanel.w ||
                             my < helpPanel.y || my >= helpPanel.y + helpPanel.h)
                        closeHelp();
                    break;
                }
                if (my >= MENU_HEIGHT && my < SCREEN_HEIGHT - SCROLLBAR_W &&
                    mx >= SCREEN_WIDTH - SCROLLBAR_W && mx < SCREEN_WIDTH)
                {
                    beginScrollDrag(1, mx, my);
                    break;
                }
                if (my >= SCREEN_HEIGHT - SCROLLBAR_W && my < SCREEN_HEIGHT &&
                    mx >= 0 && mx < SCREEN_WIDTH - SCROLLBAR_W)
                {
                    beginScrollDrag(2, mx, my);
                    break;
                }
                if (my < MENU_HEIGHT)
                {
                    handleMenuClick(mx, my);
                }
                else if (my >= MENU_HEIGHT + STATUS_STRIP_H)
                {
                    int wx = toWorldX(mx), wy = toWorldY(my);
                    if (tool == 6)
                    {
                        canvas.floodFill(wx, wy, color);
                        dirty = true;
                    }
                    else
                    {
                        mode = 1;
                        drawPoint(wx, wy);
                        if (tool >= 3 && tool <= 5)
                        {
                            if (shapeStart.x != -1)
                            {
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
                                shapeStart = {-1, -1};
                            }
                            else
                            {
                                shapeStart = {wx, wy};
                            }
                        }
                        else
                        {
                            lastWorldX = wx;
                            lastWorldY = wy;
                        }
                    }
                }
                break;
            }
            case SDL_BUTTON_MIDDLE:
                panning = true;
                panStartMX = e.button.x;
                panStartMY = e.button.y;
                panStartCamX = camX;
                panStartCamY = camY;
                break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT)
            {
                mode = 0;
                scrollDrag = 0;
            }
            else if (e.button.button == SDL_BUTTON_MIDDLE)
                panning = false;
            break;

        case SDL_MOUSEMOTION:
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
            if (scrollDrag == 1)
            {
                float viewH = (float)(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;
                float worldH = (float)canvas.height();
                float trackLen = (float)(SCREEN_HEIGHT - MENU_HEIGHT - SCROLLBAR_W);
                float thumbLen = std::max((float)MIN_THUMB_LEN, trackLen * viewH / worldH);
                float maxScroll = worldH - viewH;
                float dy = (float)(lastMouseY - scrollDragStartY);
                camY = scrollDragStartCamY + dy * maxScroll / (trackLen - thumbLen);
                camY = std::clamp(camY, (float)canvas.top(), (float)canvas.top() + maxScroll);
                dirty = true;
            }
            else if (scrollDrag == 2)
            {
                float viewW = (float)SCREEN_WIDTH / zoom;
                float worldW = (float)canvas.width();
                float trackLen = (float)(SCREEN_WIDTH - SCROLLBAR_W);
                float thumbLen = std::max((float)MIN_THUMB_LEN, trackLen * viewW / worldW);
                float maxScroll = worldW - viewW;
                float dx = (float)(lastMouseX - scrollDragStartX);
                camX = scrollDragStartCamX + dx * maxScroll / (trackLen - thumbLen);
                camX = std::clamp(camX, (float)canvas.left(), (float)canvas.left() + maxScroll);
                dirty = true;
            }
            else if (panning)
            {
                camX = panStartCamX - (float)(lastMouseX - panStartMX) / zoom;
                camY = panStartCamY - (float)(lastMouseY - panStartMY) / zoom;
                clampCamera();
                dirty = true;
            }
            else if (mode == 1 && tool <= 2 && lastMouseY >= MENU_HEIGHT + STATUS_STRIP_H)
            {
                int wx = toWorldX(lastMouseX), wy = toWorldY(lastMouseY);
                if (wx != lastWorldX || wy != lastWorldY)
                {
                    drawLine(lastWorldX, lastWorldY, wx, wy);
                    lastWorldX = wx;
                    lastWorldY = wy;
                }
            }
            break;

        case SDL_MOUSEWHEEL:
            if (SDL_GetModState() & KMOD_CTRL)
            {
                zoomAt(lastMouseX, lastMouseY, e.wheel.y > 0 ? 1.0f / 1.1f : 1.1f);
            }
            else if (SDL_GetModState() & KMOD_SHIFT)
            {
                camX -= e.wheel.y * SCROLL_PAN / zoom;
                clampCamera();
                dirty = true;
            }
            else
            {
                camY -= e.wheel.y * SCROLL_PAN / zoom;
                clampCamera();
                dirty = true;
            }
            break;

        case SDL_KEYDOWN:
            handleKey(e.key);
            break;
        }
    }
}

void PaintApp::setCursor(int type)
{
    const char *file = nullptr;
    int hotX = 0, hotY = 0;
    bool hotspotCenter = false;
    switch (type)
    {
    case 0: // pencil tip (bottom-left of the diagonal icon)
        file = "pencil.bmp";
        break;
    case 1: // crosshair center
        file = "point.bmp";
        hotspotCenter = true;
        break;
    case 2: // eraser
        file = "eraser.bmp";
        hotspotCenter = true;
        break;
    case 3: // fill (paint bucket)
        file = "fill.bmp";
        hotspotCenter = true;
        break;
    default:
        SDL_SetCursor(SDL_GetDefaultCursor());
        return;
    }

    SDL_Surface *icon = SDL_LoadBMP(assetPath(file).c_str());
    if (icon == nullptr)
        return; // asset missing - keep the default cursor

    SDL_Surface *cursorSurf = icon;
    int size = std::max(24, effectiveThickness() * 3);
    if (size != icon->w || size != icon->h)
    {
        SDL_Surface *conv = SDL_ConvertSurfaceFormat(icon, SDL_PIXELFORMAT_ARGB8888, 0);
        if (conv != nullptr)
        {
            cursorSurf = SDL_CreateRGBSurfaceWithFormat(
                0, size, size, 32, SDL_PIXELFORMAT_ARGB8888);
            for (int y = 0; y < size; ++y)
            {
                const uint8_t *s = (const uint8_t *)conv->pixels +
                                   (size_t)(y * conv->h / size) * conv->pitch;
                uint8_t *d = (uint8_t *)cursorSurf->pixels + (size_t)y * cursorSurf->pitch;
                for (int x = 0; x < size; ++x)
                {
                    const uint8_t *sp = s + (size_t)(x * conv->w / size) * 4;
                    uint8_t *dp = d + (size_t)x * 4;
                    dp[0] = sp[0];
                    dp[1] = sp[1];
                    dp[2] = sp[2];
                    dp[3] = sp[3];
                }
            }
            SDL_FreeSurface(conv);
        }
    }

    if (hotspotCenter)
    {
        hotX = (cursorSurf != nullptr ? cursorSurf->w : size) / 2;
        hotY = (cursorSurf != nullptr ? cursorSurf->h : size) / 2;
    }
    else
    {
        // Pencil: hotspot at the tip (bottom-left corner of the icon).
        hotX = 0;
        hotY = (cursorSurf != nullptr ? cursorSurf->h : size) - 1;
    }

    SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurf, hotX, hotY);
    if (cursor != nullptr)
        SDL_SetCursor(cursor);
    if (cursorSurf != icon)
        SDL_FreeSurface(cursorSurf);
    SDL_FreeSurface(icon);
}

void PaintApp::setCursorForTool()
{
    if (tool == 2)
        setCursor(2); // eraser
    else if (tool == 1)
        setCursor(0); // pencil
    else if (tool == 6)
        setCursor(3); // fill
    else
        setCursor(1); // crosshair
}

void PaintApp::handleMenuClick(int mx, int my)
{
    shapeStart = {-1, -1};

    // Tools: left-aligned cluster.
    for (const ToolSlot &slot : toolSlots)
    {
        int x0 = slot.col * TOOL_WIDTH;
        int y0 = slot.row * ROW_HEIGHT;
        if (mx < x0 || mx >= x0 + TOOL_WIDTH || my < y0 || my >= y0 + ROW_HEIGHT)
            continue;
        tool = slot.tool;
        if (slot.forceColor)
            color = colors[slot.forceColorIndex];
        setCursorForTool();
        return;
    }

    // Colors: right-aligned block.
    for (int i = 0; i < NUM_COLORS; ++i)
    {
        int row = i / MENU_COLORS_PER_ROW;
        int col = i % MENU_COLORS_PER_ROW;
        int x0 = MENU_COLOR_LEFT + col * TOOL_WIDTH;
        int y0 = row * ROW_HEIGHT;
        if (mx < x0 || mx >= x0 + TOOL_WIDTH || my < y0 || my >= y0 + ROW_HEIGHT)
            continue;
        color = colors[menuColorOrder[i]];
        return;
    }
}

}
