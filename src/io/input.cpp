#include <SDL3/SDL.h>

#include <algorithm>

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

void PaintApp::beginScrollDrag(ScrollDrag axis, int mx, int my)
{
    bool vertical = axis == ScrollDrag::Vertical;
    float viewLen = vertical ? viewHeight() : viewWidth();
    float trackLen = vertical ? verticalTrackLen() : horizontalTrackLen();
    int worldLen = vertical ? canvas.height() : canvas.width();

    if (worldLen <= viewLen)
    {
        scrollDrag = ScrollDrag::None;
        return;
    }

    ScrollMetrics m = scrollMetrics(static_cast<float>(worldLen), viewLen, trackLen);
    int mousePos = vertical ? my : mx;
    float frac = std::clamp((mousePos - (vertical ? MENU_HEIGHT : 0) - m.thumbLen / 2) /
                                (trackLen - m.thumbLen),
                            0.0f, 1.0f);
    if (vertical)
    {
        camY = static_cast<float>(canvas.top()) + frac * m.maxScroll;
        scrollDragStartY = my;
        scrollDragStartCamY = camY;
    }
    else
    {
        camX = static_cast<float>(canvas.left()) + frac * m.maxScroll;
        scrollDragStartX = mx;
        scrollDragStartCamX = camX;
    }
    scrollDrag = axis;
    dirty = true;
}

void PaintApp::handleKey(const SDL_KeyboardEvent &key)
{
    {
        std::lock_guard<std::mutex> lock(saveDialogMutex);
        if (saveDialogOpen)
            return; // a save dialog is up: keys belong to it, not to shortcuts
    }
    if (key.mod & SDL_KMOD_CTRL)
    {
        switch (key.key)
        {
        case SDLK_S:
            openSaveDialog();
            break;
        case SDLK_G:
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

    if (key.key >= SDLK_1 && key.key <= SDLK_7)
        color = colors[key.key - SDLK_1];

    switch (key.key)
    {
    case SDLK_C:
        clearScreen();
        break;
    case SDLK_Z:
        thickness = std::clamp(thickness + 1, 1, MAX_LINE_THICKNESS);
        setCursorForTool();
        break;
    case SDLK_X:
        thickness = std::clamp(thickness - 1, 1, MAX_LINE_THICKNESS);
        setCursorForTool();
        break;
    case SDLK_F:
        tool = Tool::Fill;
        setCursorForTool();
        break;
    case SDLK_ESCAPE:
        if (helpOpen)
            closeHelp();
        break;
    case SDLK_LEFT:
        camX -= ARROW_PAN_PX / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_RIGHT:
        camX += ARROW_PAN_PX / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_UP:
        camY -= ARROW_PAN_PX / zoom;
        clampCamera();
        dirty = true;
        break;
    case SDLK_DOWN:
        camY += ARROW_PAN_PX / zoom;
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
        case SDL_EVENT_QUIT:
            quit = true;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch (e.button.button)
            {
            case SDL_BUTTON_LEFT:
            {
                int mx = e.button.x, my = e.button.y;
                lastMouseX = mx;
                lastMouseY = my;
                if (helpOpen)
                {
                    // A click closes the popup or is swallowed.
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
                    beginScrollDrag(ScrollDrag::Vertical, mx, my);
                    break;
                }
                if (my >= SCREEN_HEIGHT - SCROLLBAR_W && my < SCREEN_HEIGHT &&
                    mx >= 0 && mx < SCREEN_WIDTH - SCROLLBAR_W)
                {
                    beginScrollDrag(ScrollDrag::Horizontal, mx, my);
                    break;
                }
                if (my < MENU_HEIGHT)
                {
                    handleMenuClick(mx, my);
                }
                else if (my >= MENU_HEIGHT + STATUS_STRIP_H)
                {
                    bool shapeTool = tool >= Tool::Line && tool <= Tool::Rectangle;
                    int wx = toWorldX(mx), wy = toWorldY(my);
                    if (tool == Tool::Fill)
                    {
                        canvas.floodFill(wx, wy, color);
                        dirty = true;
                    }
                    else
                    {
                        drawing = true;
                        drawPoint(wx, wy);
                        if (shapeTool)
                        {
                            if (shapeStart.x != -1)
                            {
                                drawShape(shapeStart.x, shapeStart.y, wx, wy);
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

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT)
            {
                drawing = false;
                scrollDrag = ScrollDrag::None;
            }
            else if (e.button.button == SDL_BUTTON_MIDDLE)
                panning = false;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
            if (scrollDrag != ScrollDrag::None)
            {
                bool vertical = scrollDrag == ScrollDrag::Vertical;
                float viewLen = vertical ? viewHeight() : viewWidth();
                float trackLen = vertical ? verticalTrackLen() : horizontalTrackLen();
                int worldLen = vertical ? canvas.height() : canvas.width();
                ScrollMetrics m = scrollMetrics(static_cast<float>(worldLen), viewLen, trackLen);
                float delta = static_cast<float>(vertical ? lastMouseY - scrollDragStartY
                                                          : lastMouseX - scrollDragStartX);
                float cam = scrollDrag == ScrollDrag::Vertical ? scrollDragStartCamY
                                                              : scrollDragStartCamX;
                cam += delta * m.maxScroll / (trackLen - m.thumbLen);
                float minCam = static_cast<float>(vertical ? canvas.top() : canvas.left());
                cam = std::clamp(cam, minCam, minCam + m.maxScroll);
                if (vertical)
                    camY = cam;
                else
                    camX = cam;
                dirty = true;
            }
            else if (panning)
            {
                camX = panStartCamX - static_cast<float>(lastMouseX - panStartMX) / zoom;
                camY = panStartCamY - static_cast<float>(lastMouseY - panStartMY) / zoom;
                clampCamera();
                dirty = true;
            }
            else if (drawing && (tool == Tool::Pencil || tool == Tool::Eraser) &&
                     lastMouseY >= MENU_HEIGHT + STATUS_STRIP_H)
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

        case SDL_EVENT_MOUSE_WHEEL:
            if (SDL_GetModState() & SDL_KMOD_CTRL)
            {
                zoomAt(lastMouseX, lastMouseY,
                       e.wheel.y > 0 ? ZOOM_STEP : 1.0f / ZOOM_STEP);
            }
            else if (SDL_GetModState() & SDL_KMOD_SHIFT)
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

        case SDL_EVENT_KEY_DOWN:
            handleKey(e.key);
            break;
        }
    }
}

void PaintApp::setCursor(int type)
{
    const char *file = nullptr;
    bool hotspotCenter = false;
    switch (type)
    {
    case 0: // pencil tip sits at the icon bottom-left
        file = "pencil.bmp";
        break;
    case 1:
        file = "point.bmp";
        hotspotCenter = true;
        break;
    case 2:
        file = "eraser.bmp";
        hotspotCenter = true;
        break;
    case 3:
        file = "fill.bmp";
        hotspotCenter = true;
        break;
    default:
        if (cursor != nullptr)
        {
            SDL_DestroyCursor(cursor);
            cursor = nullptr;
        }
        SDL_SetCursor(SDL_GetDefaultCursor());
        return;
    }

    SurfacePtr icon{SDL_LoadBMP(assetPath(file).c_str())};
    if (icon == nullptr)
        return; // asset missing; keep current cursor

    int size = std::max(CURSOR_MIN_SIZE, effectiveThickness() * CURSOR_THICKNESS_SCALE);
    SDL_Surface *cursorSurf = icon.get();
    SurfacePtr scaled;
    if (size != icon->w || size != icon->h)
    {
        SurfacePtr conv{SDL_ConvertSurface(icon.get(), SDL_PIXELFORMAT_ARGB8888)};
        if (conv != nullptr)
        {
            scaled.reset(SDL_CreateSurface(size, size, SDL_PIXELFORMAT_ARGB8888));
            if (scaled != nullptr)
            {
                for (int y = 0; y < size; ++y)
                {
                    const auto *s = static_cast<const uint8_t *>(conv->pixels) +
                                    static_cast<size_t>(y * conv->h / size) * conv->pitch;
                    auto *d = static_cast<uint8_t *>(scaled->pixels) +
                              static_cast<size_t>(y) * scaled->pitch;
                    for (int x = 0; x < size; ++x)
                    {
                        const uint8_t *sp =
                            s + static_cast<size_t>(x * conv->w / size) * 4;
                        uint8_t *dp = d + static_cast<size_t>(x) * 4;
                        dp[0] = sp[0];
                        dp[1] = sp[1];
                        dp[2] = sp[2];
                        dp[3] = sp[3];
                    }
                }
                cursorSurf = scaled.get();
            }
        }
    }

    int hotX, hotY;
    if (hotspotCenter)
    {
        hotX = cursorSurf->w / 2;
        hotY = cursorSurf->h / 2;
    }
    else
    {
        hotX = 0;
        hotY = cursorSurf->h - 1;
    }

    SDL_Cursor *next = SDL_CreateColorCursor(cursorSurf, hotX, hotY);
    if (next == nullptr)
        return;
    if (cursor != nullptr)
        SDL_DestroyCursor(cursor);
    cursor = next;
    SDL_SetCursor(cursor);
}

void PaintApp::setCursorForTool()
{
    switch (tool)
    {
    case Tool::Pencil:
        setCursor(0);
        break;
    case Tool::Eraser:
        setCursor(2);
        break;
    case Tool::Fill:
        setCursor(3);
        break;
    default:
        setCursor(1); // crosshair
        break;
    }
}

void PaintApp::handleMenuClick(int mx, int my)
{
    shapeStart = {-1, -1};

    // Tool cluster, left side.
    for (const ToolSlot &slot : toolSlots)
    {
        SDL_Rect r = toolSlotRect(slot);
        if (mx < r.x || mx >= r.x + r.w || my < r.y || my >= r.y + r.h)
            continue;
        tool = slot.tool;
        if (slot.forceColor)
            color = colors[slot.forceColorIndex];
        setCursorForTool();
        return;
    }

    for (int i = 0; i < NUM_COLORS; ++i)
    {
        SDL_Rect r = menuColorRect(i);
        if (mx < r.x || mx >= r.x + r.w || my < r.y || my >= r.y + r.h)
            continue;
        color = colors[menuColorOrder[i]];
        return;
    }
}

}
