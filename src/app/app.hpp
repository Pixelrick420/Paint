#pragma once

#include <SDL2/SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#define HAS_SDL_TTF 1
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include "common.hpp"
#include "draw/canvas.hpp"
#include "io/bmp.hpp"

namespace paint
{

// Owning handle to an SDL_Surface freed on scope exit.
struct SDLSurfaceDeleter
{
    void operator()(SDL_Surface *surface) const { SDL_FreeSurface(surface); }
};

using SurfacePtr = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

// Which scroll bar (if any) is being dragged.
enum class ScrollDrag
{
    None = 0,
    Vertical,
    Horizontal,
};

// Visible world-space rectangle: [l, r) x [t, b).
struct ViewRect
{
    float l, t, r, b;
};

// Thumb geometry for one scroll bar.
struct ScrollMetrics
{
    float thumbLen;
    float maxScroll;
};

class PaintApp
{
private:
    // SDL resources ---------------------------------------------------------
    SDL_Window *window{nullptr};
    SDL_Renderer *renderer{nullptr};
    SDL_Texture *canvasTex{nullptr};
    SDL_Texture *menuTex{nullptr};
    int gridMax{MAX_GRID};

    // Help popup (opened with Ctrl+/)
#ifdef HAS_SDL_TTF
    TTF_Font *helpFont{nullptr};
#endif
    SDL_Texture *helpTex{nullptr};      // body text
    SDL_Texture *helpTitleTex{nullptr}; // "Help" title
    SDL_Rect helpPanel{0, 0, 0, 0};     // popup panel rect (screen coords)
    SDL_Rect helpClose{0, 0, 0, 0};     // close-button rect (screen coords)
    bool helpOpen{false};

    // Pending stroke points (world coords) until the next texture upload.
    std::deque<SDL_Point> points;
    std::vector<uint8_t> fillBuf;
    std::vector<uint8_t> uploadBuf; // RGBA -> ABGR texture staging buffer
    bool dirty{true};               // canvas or view changed since last texture upload
    SDL_Rect lastClip{0, 0, -1, -1};

    // Camera / world
    float camX{0}, camY{0}, zoom{1.0f};

    // Input / interaction state
    int lastMouseX{0}, lastMouseY{0};
    int lastWorldX{0}, lastWorldY{0};
    bool panning{false};
    int panStartMX{0}, panStartMY{0};
    float panStartCamX{0}, panStartCamY{0};

    ScrollDrag scrollDrag{ScrollDrag::None};
    int scrollDragStartX{0}, scrollDragStartY{0};
    float scrollDragStartCamX{0}, scrollDragStartCamY{0};

    Color color{0, 0, 0};
    bool drawing{false}; // left mouse button held on the canvas
    Tool tool{Tool::Pencil};
    int thickness{2};
    bool quit{false};
    bool showGrid{false};
    bool previewing{false};
    SDL_Point shapeStart = {-1, -1};
    Uint32 saveFlashUntil{0};

    Canvas canvas;

    // View / coordinate helpers ---------------------------------------------
    [[nodiscard]] float viewWidth() const { return static_cast<float>(SCREEN_WIDTH) / zoom; }

    [[nodiscard]] float viewHeight() const
    {
        return static_cast<float>(SCREEN_HEIGHT - MENU_HEIGHT) / zoom;
    }

    [[nodiscard]] ViewRect viewRect() const
    {
        return {camX, camY, camX + viewWidth(), camY + viewHeight()};
    }

    [[nodiscard]] int effectiveThickness() const
    {
        return std::max(1, static_cast<int>(std::lround(thickness * zoom)));
    }

    [[nodiscard]] int toWorldX(int sx) const
    {
        return static_cast<int>(std::floor(camX + sx / zoom));
    }

    [[nodiscard]] int toWorldY(int sy) const
    {
        return static_cast<int>(std::floor(camY + (sy - MENU_HEIGHT) / zoom));
    }

    void clampCamera()
    {
        camX = std::clamp(camX, static_cast<float>(canvas.left()) - viewWidth(),
                          static_cast<float>(canvas.right()));
        camY = std::clamp(camY, static_cast<float>(canvas.top()) - viewHeight(),
                          static_cast<float>(canvas.bottom()));
    }

    [[nodiscard]] static SDL_Rect toolSlotRect(const ToolSlot &slot)
    {
        return {slot.col * TOOL_WIDTH, slot.row * ROW_HEIGHT, TOOL_WIDTH, ROW_HEIGHT};
    }

    [[nodiscard]] static SDL_Rect menuColorRect(int i)
    {
        return {MENU_COLOR_LEFT + (i % MENU_COLORS_PER_ROW) * TOOL_WIDTH,
                (i / MENU_COLORS_PER_ROW) * ROW_HEIGHT, TOOL_WIDTH, ROW_HEIGHT};
    }

    // Thumb length / travel range for a scroll bar over worldLen with trackLen.
    [[nodiscard]] static ScrollMetrics scrollMetrics(float worldLen, float viewLen,
                                                     float trackLen)
    {
        float thumbLen = std::max(static_cast<float>(MIN_THUMB_LEN), trackLen * viewLen / worldLen);
        return {thumbLen, worldLen - viewLen};
    }

    [[nodiscard]] float verticalTrackLen() const
    {
        return static_cast<float>(SCREEN_HEIGHT - MENU_HEIGHT - SCROLLBAR_W);
    }

    [[nodiscard]] float horizontalTrackLen() const
    {
        return static_cast<float>(SCREEN_WIDTH - SCROLLBAR_W);
    }

    // Dispatch to line/circle/rectangle based on the current shape tool.
    void drawShape(int x1, int y1, int x2, int y2);

    // Drawing tools (draw.cpp) ----------------------------------------------
    void drawPoint(int wx, int wy);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawCircle(int cx, int cy, int x, int y);
    void drawRectangle(int x1, int y1, int x2, int y2);
    void flushPoints();
    void saveCanvasBMP();
    void ensureCanvasCoversView();

    // Rendering (render.cpp) --------------------------------------------------
    void setDrawColor(uint8_t grey);
    void drawCanvasView();
    void drawGrid();
    [[nodiscard]] SDL_Rect canvasScreenRect() const;
    void drawStatusStrip();
    void drawPreview();
    void drawFlashOverlays();
    void drawScrollBars();
    void drawScreen();

    // Help popup (app.cpp) -----------------------------------------------------
    void buildHelpTexture();
    void drawHelpPopup();
    void toggleHelp() { helpOpen = !helpOpen; }
    void closeHelp() { helpOpen = false; }

    // Input (input.cpp) ----------------------------------------------------------
    void zoomAt(int mx, int my, float factor);
    void beginScrollDrag(ScrollDrag axis, int mx, int my);
    void handleKey(const SDL_KeyboardEvent &key);
    void handleInput();
    void setCursor(int type);
    void setCursorForTool();
    void handleMenuClick(int mx, int my);

    // Lifecycle (app.cpp) ---------------------------------------------------------
    [[nodiscard]] SDL_Texture *buildMenuTexture();
    void clearScreen();

public:
    PaintApp();
    ~PaintApp();

    void run();
    void smokeTest();
};

}
