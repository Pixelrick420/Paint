#pragma once

#include <SDL3/SDL.h>
#if __has_include(<SDL3_ttf/SDL_ttf.h>)
#include <SDL3_ttf/SDL_ttf.h>
#define HAS_SDL_TTF 1
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common.hpp"
#include "draw/canvas.hpp"
#include "io/bmp.hpp"

namespace paint
{

struct SDLSurfaceDeleter
{
    void operator()(SDL_Surface *surface) const { SDL_DestroySurface(surface); }
};

using SurfacePtr = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

enum class ScrollDrag
{
    None = 0,
    Vertical,
    Horizontal,
};

// Visible world rect: [l,r) x [t,b).
struct ViewRect
{
    float l, t, r, b;
};

struct ScrollMetrics
{
    float thumbLen;
    float maxScroll;
};

class PaintApp
{
private:
    SDL_Window *window{nullptr};
    SDL_Renderer *renderer{nullptr};
    SDL_Texture *canvasTex{nullptr};
    SDL_Texture *menuTex{nullptr};
    int gridMax{MAX_GRID};

    // Help popup (Ctrl+/)
#ifdef HAS_SDL_TTF
    TTF_Font *helpFont{nullptr};
#endif
    SDL_Texture *helpTex{nullptr};
    SDL_Texture *helpTitleTex{nullptr};
    SDL_Rect helpPanel{0, 0, 0, 0};
    SDL_Rect helpClose{0, 0, 0, 0};
    bool helpOpen{false};

    // Stroke points in world coords, pending texture upload.
    std::deque<SDL_Point> points;
    std::vector<uint8_t> fillBuf;
    std::vector<uint8_t> uploadBuf; // staging buffer; swizzles RGBA to ABGR
    bool dirty{true};               // canvas or view changed since last upload
    SDL_Rect lastClip{0, 0, -1, -1};

    float camX{0}, camY{0}, zoom{1.0f};

    int lastMouseX{0}, lastMouseY{0};
    SDL_Cursor *cursor{nullptr}; // app-created; destroyed on replace/exit
    int lastWorldX{0}, lastWorldY{0};
    bool panning{false};
    int panStartMX{0}, panStartMY{0};
    float panStartCamX{0}, panStartCamY{0};

    ScrollDrag scrollDrag{ScrollDrag::None};
    int scrollDragStartX{0}, scrollDragStartY{0};
    float scrollDragStartCamX{0}, scrollDragStartCamY{0};

    Color color{colors[BLACK]};
    bool drawing{false}; // left button held on canvas
    Tool tool{Tool::Pencil};
    int thickness{2};
    bool quit{false};
    bool showGrid{false};
    bool previewing{false};
    SDL_Point shapeStart = {-1, -1};
    Uint32 saveFlashUntil{0};

    // Save-dialog results arrive on a callback thread; the main loop consumes them.
    std::mutex saveDialogMutex;
    std::string saveDialogResult; // chosen path, empty when the dialog was cancelled
    bool saveDialogDone{false};

    Canvas canvas;

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

    void drawShape(int x1, int y1, int x2, int y2);

    void drawPoint(int wx, int wy);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawCircle(int cx, int cy, int x, int y);
    void drawRectangle(int x1, int y1, int x2, int y2);
    void flushPoints();
    void saveCanvasBMP();
    bool writeTo(const std::string &path);
    void openSaveDialog();
    void processSaveDialog();
    [[nodiscard]] bool confirmOverwrite(const std::string &path) const;
    static void SDLCALL onSaveDialogDone(void *userdata, const char *const *filelist,
                                         int filter);
    void ensureCanvasCoversView();

    void setDrawColor(uint8_t grey);
    void drawCanvasView();
    void drawGrid();
    [[nodiscard]] SDL_Rect canvasScreenRect() const;
    void drawStatusStrip();
    void drawPreview();
    void drawFlashOverlays();
    void drawScrollBars();
    void drawScreen();

    void buildHelpTexture();
    void drawHelpPopup();
    void toggleHelp() { helpOpen = !helpOpen; }
    void closeHelp() { helpOpen = false; }

    void zoomAt(int mx, int my, float factor);
    void beginScrollDrag(ScrollDrag axis, int mx, int my);
    void handleKey(const SDL_KeyboardEvent &key);
    void handleInput();
    void setCursor(int type);
    void setCursorForTool();
    void handleMenuClick(int mx, int my);

    [[nodiscard]] SDL_Texture *buildMenuTexture();
    void clearScreen();

public:
    PaintApp();
    ~PaintApp();

    void run();
    void smokeTest();
};

}
