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
    dirty = true;
    setCursorForTool();
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
    case SDLK_LEFT:
        camX -= 40.0f / zoom;
        dirty = true;
        break;
    case SDLK_RIGHT:
        camX += 40.0f / zoom;
        dirty = true;
        break;
    case SDLK_UP:
        camY -= 40.0f / zoom;
        dirty = true;
        break;
    case SDLK_DOWN:
        camY += 40.0f / zoom;
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
                if (my < MENU_HEIGHT)
                {
                    handleToolSelection(my / ROW_HEIGHT, mx / TOOL_WIDTH);
                }
                else if (my >= MENU_HEIGHT + STATUS_STRIP_H)
                {
                    int wx = toWorldX(mx), wy = toWorldY(my);
                    if (tool == 6)
                    {
                        canvas.floodFill(wx, wy, color);
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
                mode = 0;
            else if (e.button.button == SDL_BUTTON_MIDDLE)
                panning = false;
            break;

        case SDL_MOUSEMOTION:
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
            if (panning)
            {
                camX = panStartCamX - (float)(lastMouseX - panStartMX) / zoom;
                camY = panStartCamY - (float)(lastMouseY - panStartMY) / zoom;
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
            zoomAt(lastMouseX, lastMouseY, e.wheel.y > 0 ? 1.1f : 1.0f / 1.1f);
            break;

        case SDL_KEYDOWN:
            handleKey(e.key);
            break;
        }
    }
}

void PaintApp::setCursor(int type)
{
    std::string cursorFile;
    switch (type)
    {
    case 0:
        cursorFile = assetPath("pencil.bmp");
        break;
    case 1:
        cursorFile = assetPath("point.bmp");
        break;
    case 2:
        cursorFile = assetPath("eraser.bmp");
        break;
    default:
        SDL_SetCursor(SDL_GetDefaultCursor());
        return;
    }
    int cursorSize = effectiveThickness() * 3;
    SDL_Surface *cursorSurface = SDL_LoadBMP(cursorFile.c_str());
    if (cursorSurface == nullptr)
    {
        return; // asset missing - keep the default cursor
    }
    SDL_Cursor *cursor = nullptr;
    if (type == 2)
    {
        SDL_Surface *scaledSurface = SDL_CreateRGBSurfaceWithFormat(
            0, cursorSize, cursorSize, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_Rect srcRect = {0, 0, cursorSurface->w, cursorSurface->h};
        SDL_Rect dstRect = {0, 0, cursorSize, cursorSize};
        SDL_BlitScaled(cursorSurface, &srcRect, scaledSurface, &dstRect);
        cursor = SDL_CreateColorCursor(scaledSurface, cursorSize / 2, cursorSize / 2);
        SDL_FreeSurface(scaledSurface);
    }
    else
    {
        cursor = SDL_CreateColorCursor(cursorSurface, 0, cursorSurface->h - 1);
    }
    SDL_SetCursor(cursor);
    SDL_FreeSurface(cursorSurface);
}

void PaintApp::setCursorForTool()
{
    if (tool == 2)
        setCursor(2);
    else if (tool == 1)
        setCursor(0);
    else
        setCursor(1);
}

void PaintApp::handleToolSelection(int row, int col)
{
    shapeStart = {-1, -1};
    if (row == 0)
    {
        switch (col)
        {
        case 0:
            tool = 1;
            color = colors[0];
            setCursorForTool();
            break; // Pencil
        case 1:
            tool = 3;
            setCursorForTool();
            break; // Line
        case 2:
            tool = 5;
            setCursorForTool();
            break; // Rectangle
        case 3:
            color = colors[0];
            break;
        case 4:
            color = colors[2];
            break;
        case 5:
            color = colors[4];
            break;
        case 6:
            color = colors[6];
            break;
        case 7:
            tool = 6;
            setCursorForTool();
            break; // Fill
        }
    }
    else if (row == 1)
    {
        switch (col)
        {
        case 0:
            tool = 2;
            color = colors[7];
            setCursorForTool();
            break; // Eraser
        case 1:
            tool = 4;
            setCursorForTool();
            break; // Circle
        case 2:
            printHelp();
            break; // Help
        case 3:
            color = colors[1];
            break;
        case 4:
            color = colors[3];
            break;
        case 5:
            color = colors[5];
            break;
        case 6:
            color = colors[7];
            break;
        }
    }
}

}
