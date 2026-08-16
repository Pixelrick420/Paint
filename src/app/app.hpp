#pragma once

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>

#include "common.hpp"
#include "draw/canvas.hpp"
#include "io/bmp.hpp"

namespace paint
{

class PaintApp
{
private:
    // SDL resources ---------------------------------------------------------
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *canvasTex;
    SDL_Texture *menuTex;
    SDL_Texture *fillTex;
    SDL_Surface *fillSurf;
    int fillTexW{0}, fillTexH{0};
    int gridMax{MAX_GRID};

    // Pending stroke points (world coords) until the next texture upload.
    std::deque<SDL_Point> points;
    std::vector<uint8_t> fillBuf;
    std::vector<uint8_t> uploadBuf; // RGBA -> ABGR texture staging buffer
    bool dirty{true}; // canvas or view changed since last texture upload
    SDL_Rect lastClip{0, 0, -1, -1};

    // Camera / world
    float camX{0}, camY{0}, zoom{1.0f};

    // Input / interaction state
    int lastMouseX{0}, lastMouseY{0};
    int lastWorldX{0}, lastWorldY{0};
    bool panning{false};
    int panStartMX{0}, panStartMY{0};
    float panStartCamX{0}, panStartCamY{0};

    int scrollDrag{0}; // 0 = none, 1 = vertical bar, 2 = horizontal bar
    int scrollDragStartX{0}, scrollDragStartY{0};
    float scrollDragStartCamX{0}, scrollDragStartCamY{0};

    Color color{0, 0, 0};
    int mode{0}; // 0 = not clicking, 1 = dragging a tool
    int tool{1}; // 1=pencil, 2=eraser, 3=line, 4=circle, 5=rectangle, 6=fill
    int thickness{2};
    bool quit{false};
    bool showGrid{false};
    bool previewing{false};
    SDL_Point shapeStart = {-1, -1};
    Uint32 saveFlashUntil{0};
    Uint32 thickFlashUntil{0};

    Canvas canvas;

    // Coordinate helpers ----------------------------------------------------
    static int floorDiv(int a, int b)
    {
        int q = a / b;
        if (a % b != 0 && ((a < 0) != (b < 0)))
            --q;
        return q;
    }

    static int ceilDiv(int a, int b)
    {
        return -floorDiv(-a, b);
    }

    int effectiveThickness() const
    {
        return std::max(1, static_cast<int>(std::lround(thickness * zoom)));
    }

    int toWorldX(int sx) const
    {
        return static_cast<int>(std::floor(camX + sx / zoom));
    }

    int toWorldY(int sy) const
    {
        return static_cast<int>(std::floor(camY + (sy - MENU_HEIGHT) / zoom));
    }

    // Drawing tools (paint_draw.cpp) ----------------------------------------
    void drawPoint(int wx, int wy);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawCircle(int cx, int cy, int x, int y);
    void drawRectangle(int x1, int y1, int x2, int y2);
    void flushPoints();
    void saveCanvasBMP();
    void ensureCanvasCoversView();

    // Rendering (paint_render.cpp) ------------------------------------------
    void drawCanvasView();
    void drawGrid();
    SDL_Rect canvasScreenRect() const;
    void drawFillIcon();
    void drawStatusStrip();
    void drawPreview();
    void drawFlashOverlays();
    void drawScrollBars();
    void drawScreen();

    // Input (paint_input.cpp) -----------------------------------------------
    void zoomAt(int mx, int my, float factor);
    void beginScrollDrag(int axis, int mx, int my);
    void handleKey(const SDL_KeyboardEvent &key);
    void handleInput();
    void setCursor(int type);
    void setCursorForTool();
    void handleToolSelection(int row, int col);

    // Lifecycle (paint_app.cpp) ---------------------------------------------
    SDL_Texture *buildMenuTexture();
    void clearScreen();

public:
    PaintApp();
    ~PaintApp();

    void run();
    void smokeTest();
    static void printHelp();
};

}
