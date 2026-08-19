#include <SDL2/SDL.h>
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#define HAS_SDL_TTF 1
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app.hpp"

namespace paint
{

PaintApp::PaintApp()
    : window(nullptr), renderer(nullptr), canvasTex(nullptr),
      menuTex(nullptr)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }
    window = SDL_CreateWindow("Paint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (renderer == nullptr)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    int maxDim = std::min(info.max_texture_width, info.max_texture_height);
    if (maxDim <= 0)
        maxDim = MAX_GRID;
    gridMax = std::min(MAX_GRID, maxDim);
    gridMax = std::max(gridMax, std::max(INITIAL_CANVAS_WIDTH, INITIAL_CANVAS_HEIGHT));

    canvasTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, gridMax, gridMax);
    if (canvasTex == nullptr)
    {
        std::cerr << "SDL_CreateTexture (canvas) failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    canvas.init(gridMax, INITIAL_CANVAS_WIDTH, INITIAL_CANVAS_HEIGHT);

    menuTex = buildMenuTexture();

#ifdef HAS_SDL_TTF
    if (TTF_Init() == 0)
        buildHelpTexture();
    else
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
#endif
}

PaintApp::~PaintApp()
{
    SDL_DestroyTexture(helpTex);
    SDL_DestroyTexture(helpTitleTex);
    SDL_DestroyTexture(menuTex);
    SDL_DestroyTexture(canvasTex);
#ifdef HAS_SDL_TTF
    if (helpFont != nullptr)
        TTF_CloseFont(helpFont);
    TTF_Quit();
#endif
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

namespace
{
// Alpha-composite an ARGB surface onto dst at (cx, cy) (centered).
// Assumes dst is SDL_PIXELFORMAT_ARGB8888 and blits over an opaque white
// background, so the output alpha is always 255.
void compositeIcon(SDL_Surface *dst, SDL_Surface *icon, int cx, int cy)
{
    SDL_Surface *conv = SDL_ConvertSurfaceFormat(icon, SDL_PIXELFORMAT_ARGB8888, 0);
    if (conv == nullptr)
        return;
    int x0 = cx - conv->w / 2;
    int y0 = cy - conv->h / 2;
    for (int y = 0; y < conv->h; ++y)
    {
        int dy = y0 + y;
        if (dy < 0 || dy >= dst->h)
            continue;
        const uint8_t *s = (const uint8_t *)conv->pixels + (size_t)y * conv->pitch;
        uint8_t *d = (uint8_t *)dst->pixels + (size_t)dy * dst->pitch;
        for (int x = 0; x < conv->w; ++x)
        {
            int dx = x0 + x;
            if (dx < 0 || dx >= dst->w)
                continue;
            const uint8_t *sp = s + (size_t)x * 4; // B,G,R,A
            uint8_t *dp = d + (size_t)dx * 4;
            uint8_t a = sp[3];
            if (a == 0)
                continue;
            dp[0] = (uint16_t)sp[0] * a / 255 + dp[0] * (255 - a) / 255;
            dp[1] = (uint16_t)sp[1] * a / 255 + dp[1] * (255 - a) / 255;
            dp[2] = (uint16_t)sp[2] * a / 255 + dp[2] * (255 - a) / 255;
        }
    }
    SDL_FreeSurface(conv);
}
} // namespace

SDL_Texture *PaintApp::buildMenuTexture()
{
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, SCREEN_WIDTH, MENU_HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surf == nullptr)
        return nullptr;
    SDL_memset(surf->pixels, 255, (size_t)surf->pitch * surf->h); // opaque white

    for (const ToolSlot &slot : toolSlots)
    {
        SDL_Surface *icon = SDL_LoadBMP(assetPath(slot.icon).c_str());
        if (icon == nullptr)
            continue;
        compositeIcon(surf, icon,
                      slot.col * TOOL_WIDTH + TOOL_WIDTH / 2,
                      slot.row * ROW_HEIGHT + ROW_HEIGHT / 2);
        SDL_FreeSurface(icon);
    }

    for (int i = 0; i < NUM_COLORS; ++i)
    {
        int row = i / MENU_COLORS_PER_ROW;
        int col = i % MENU_COLORS_PER_ROW;
        int x0 = MENU_COLOR_LEFT + col * TOOL_WIDTH;
        int y0 = row * ROW_HEIGHT;
        const Color &c = colors[menuColorOrder[i]];
        uint32_t px = SDL_MapRGBA(surf->format, c.r, c.g, c.b, 255);
        uint32_t *dst = (uint32_t *)surf->pixels;
        for (int y = y0; y < y0 + ROW_HEIGHT; ++y)
            for (int x = x0; x < x0 + TOOL_WIDTH; ++x)
                dst[y * surf->w + x] = px;
    }

    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    return t;
}

void PaintApp::clearScreen()
{
    points.clear();
    color = {0, 0, 0};
    mode = 0;
    tool = 1;
    thickness = 2;
    shapeStart = {-1, -1};
    camX = 0;
    camY = 0;
    zoom = 1.0f;
    scrollDrag = 0;
    canvas.clear();
    dirty = true;
    setCursorForTool();
}

void PaintApp::run()
{
    clearScreen();
    setCursorForTool();
    lastMouseX = SCREEN_WIDTH / 2;
    lastMouseY = MENU_HEIGHT + (SCREEN_HEIGHT - MENU_HEIGHT) / 2;
    while (!quit)
    {
        handleInput();
        ensureCanvasCoversView();
        drawScreen();
        SDL_Delay(16);
    }
}

void PaintApp::smokeTest()
{
    clearScreen();
    drawLine(100, 200, 600, 400);
    drawCircle(640, 360, 800, 360);
    drawScreen();
    SDL_Delay(100);

    // TEMP: capture menu and help-popup frames for pixel verification.
    {
        std::vector<uint8_t> px((size_t)SCREEN_WIDTH * SCREEN_HEIGHT * 4);
        SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ABGR8888,
                             px.data(), SCREEN_WIDTH * 4);
        writeBMPFile("frame_menu.bmp", px.data(), SCREEN_WIDTH, SCREEN_HEIGHT);
        helpOpen = true;
        drawScreen();
        SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ABGR8888,
                             px.data(), SCREEN_WIDTH * 4);
        writeBMPFile("frame_popup.bmp", px.data(), SCREEN_WIDTH, SCREEN_HEIGHT);
        helpOpen = false;
        drawScreen();
    }

    canvas.floodFill(700, 200, color);
    drawScreen();
    SDL_Delay(100);

    camX = -200;
    camY = -150;
    zoom = 0.5f;
    ensureCanvasCoversView();
    drawScreen();
    SDL_Delay(100);

    saveCanvasBMP();
    drawScreen();
    SDL_Delay(100);
}

void PaintApp::buildHelpTexture()
{
#ifdef HAS_SDL_TTF
    static const char *fontCandidates[] = {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
        "/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    for (const char *path : fontCandidates)
    {
        if (access(path, R_OK) != 0)
            continue;
        helpFont = TTF_OpenFont(path, 14);
        if (helpFont != nullptr)
            break;
    }
    if (helpFont == nullptr)
    {
        std::cerr << "Help popup disabled: no usable font found" << std::endl;
        return;
    }

    static const char *lines[] = {
        "MOUSE",
        "- Click a tool or a color in the menu to select it.",
        "- Left-click and drag to draw with the pencil tool.",
        "- Use the eraser tool to erase parts of the drawing.",
        "- Line, circle, rectangle: click once for the start and once",
        "  for the end of the shape; a preview follows the mouse.",
        "- Fill (bucket) tool: click inside a region to flood-fill it.",
        "- Ctrl + mouse wheel: zoom in/out at the cursor.",
        "- Mouse wheel: scroll up/down.  Shift + mouse wheel: left/right.",
        "- Drag the scroll bars (right / bottom edges) to scroll the canvas.",
        "- Middle-mouse drag or arrow keys: pan the canvas.",
        "",
        "KEYBOARD",
        "- [1-7]: Select a color.",
        "- [C]: Clear the canvas.",
        "- [Z] / [X]: Increase / decrease line thickness.",
        "- [F]: Select the fill (bucket) tool.",
        "- [Ctrl+S]: Save the drawing as a BMP.",
        "- [Ctrl+G]: Toggle the grid.",
        "- [Ctrl+/]: Show this help popup.",
        "",
        "MENU",
        "Tools (left):   Row 1: Pencil, Line, Rectangle",
        "                Row 2: Eraser, Circle, Fill",
        "Colors (right): Row 1: Black, Blue, Yellow, Gray",
        "                Row 2: Purple, Green, Red, White",
        "",
        "Click the X (top-right), press Esc, or click outside the popup to close.",
        "",
        "The canvas grows in any direction as you move toward its edges,",
        "up to a maximum size, after which no new cells are generated.",
    };

    constexpr int PAD = 20;
    constexpr int HEADER = 44;
    const SDL_Color textColor{230, 230, 230, 255};
    const SDL_Color titleColor{255, 255, 255, 255};

    std::vector<SDL_Surface *> lineSurfs;
    lineSurfs.reserve(sizeof(lines) / sizeof(lines[0]));
    int bodyW = 0, bodyH = 0;
    for (const char *text : lines)
    {
        SDL_Surface *s = TTF_RenderUTF8_Blended(helpFont, text, textColor);
        if (s == nullptr)
            continue;
        bodyW = std::max(bodyW, s->w);
        bodyH += s->h + 2;
        lineSurfs.push_back(s);
    }
    bodyH -= 2;

    SDL_Surface *body = SDL_CreateRGBSurfaceWithFormat(
        0, bodyW, bodyH, 32, SDL_PIXELFORMAT_ARGB8888);
    if (body != nullptr)
    {
        SDL_memset(body->pixels, 0, (size_t)body->pitch * body->h); // transparent
        int y = 0;
        for (SDL_Surface *s : lineSurfs)
        {
            SDL_Rect dst{0, y, s->w, s->h};
            SDL_BlitSurface(s, nullptr, body, &dst);
            y += s->h + 2;
        }
    }
    for (SDL_Surface *s : lineSurfs)
        SDL_FreeSurface(s);

    if (body != nullptr)
        helpTex = SDL_CreateTextureFromSurface(renderer, body);

    TTF_SetFontSize(helpFont, 20);
    SDL_Surface *title = TTF_RenderUTF8_Blended(helpFont, "Paint - Help", titleColor);
    if (title != nullptr)
    {
        helpTitleTex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_FreeSurface(title);
    }
    TTF_SetFontSize(helpFont, 14);

    if (body != nullptr)
        SDL_FreeSurface(body);

    int titleW = 0, titleH = 0;
    if (helpTitleTex != nullptr)
        SDL_QueryTexture(helpTitleTex, nullptr, nullptr, &titleW, &titleH);

    int panelW = std::clamp(std::max(bodyW, titleW) + 2 * PAD, 0, SCREEN_WIDTH - 40);
    int panelH = std::clamp(HEADER + bodyH + 2 * PAD, 0, SCREEN_HEIGHT - 40);
    helpPanel = { (SCREEN_WIDTH - panelW) / 2, (SCREEN_HEIGHT - panelH) / 2, panelW, panelH };
    helpClose = { helpPanel.x + helpPanel.w - 34, helpPanel.y + 8, 26, 26 };
#endif
}

void PaintApp::drawHelpPopup()
{
    if (!helpOpen || helpTex == nullptr)
        return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, nullptr); // dim everything behind the popup

    // Panel.
    SDL_SetRenderDrawColor(renderer, 30, 30, 46, 255);
    SDL_RenderFillRect(renderer, &helpPanel);
    SDL_SetRenderDrawColor(renderer, 140, 140, 160, 255);
    SDL_RenderDrawRect(renderer, &helpPanel);

    // Title + body text.
    if (helpTitleTex != nullptr)
    {
        SDL_Rect dst{helpPanel.x + 20, helpPanel.y + 12, 0, 0};
        SDL_QueryTexture(helpTitleTex, nullptr, nullptr, &dst.w, &dst.h);
        SDL_RenderCopy(renderer, helpTitleTex, nullptr, &dst);
    }
    if (helpTex != nullptr)
    {
        int w = 0, h = 0;
        SDL_QueryTexture(helpTex, nullptr, nullptr, &w, &h);
        SDL_Rect dst{helpPanel.x + 20, helpPanel.y + 44, w, h};
        SDL_RenderCopy(renderer, helpTex, nullptr, &dst);
    }

    // Close button (X).
    SDL_SetRenderDrawColor(renderer, 70, 70, 92, 255);
    SDL_RenderFillRect(renderer, &helpClose);
    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_RenderDrawLine(renderer, helpClose.x + 7, helpClose.y + 7,
                       helpClose.x + 19, helpClose.y + 19);
    SDL_RenderDrawLine(renderer, helpClose.x + 19, helpClose.y + 7,
                       helpClose.x + 7, helpClose.y + 19);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}

