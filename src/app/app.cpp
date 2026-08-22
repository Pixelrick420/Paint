#include <SDL3/SDL.h>
#if __has_include(<SDL3_ttf/SDL_ttf.h>)
#include <SDL3_ttf/SDL_ttf.h>
#define HAS_SDL_TTF 1
#endif

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include <unistd.h>

#include "app.hpp"

namespace paint
{

PaintApp::PaintApp()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }
    SDL_PropertiesID wprops = SDL_CreateProperties();
    SDL_SetStringProperty(wprops, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Paint");
    SDL_SetNumberProperty(wprops, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(wprops, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(wprops, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, SCREEN_WIDTH);
    SDL_SetNumberProperty(wprops, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, SCREEN_HEIGHT);
    window = SDL_CreateWindowWithProperties(wprops);
    SDL_DestroyProperties(wprops);
    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    SDL_PropertiesID props = SDL_GetRendererProperties(renderer);
    int maxDim = static_cast<int>(
        SDL_GetNumberProperty(props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));
    if (maxDim <= 0)
        maxDim = MAX_GRID;
    gridMax = std::min(MAX_GRID, maxDim);
    gridMax = std::max(gridMax, std::max(INITIAL_CANVAS_WIDTH, INITIAL_CANVAS_HEIGHT));

    canvasTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, gridMax, gridMax);
    if (canvasTex != nullptr)
        SDL_SetTextureScaleMode(canvasTex, SDL_SCALEMODE_NEAREST); // crisp pixels when zoomed
    if (canvasTex == nullptr)
    {
        std::cerr << "SDL_CreateTexture (canvas) failed: " << SDL_GetError() << std::endl;
        std::exit(1);
    }

    canvas.init(gridMax, INITIAL_CANVAS_WIDTH, INITIAL_CANVAS_HEIGHT);

    menuTex = buildMenuTexture();

#ifdef HAS_SDL_TTF
    if (TTF_Init())
        buildHelpTexture();
    else
        std::cerr << "Help popup disabled: font library init failed" << std::endl;
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
    if (cursor != nullptr)
        SDL_DestroyCursor(cursor);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

namespace
{

// Alpha-blends icon onto dst at (cx,cy), centered.
// dst must be ARGB8888 over opaque white; output alpha stays 255.
void compositeIcon(SDL_Surface *dst, const SurfacePtr &icon, int cx, int cy)
{
    SurfacePtr conv{SDL_ConvertSurface(icon.get(), SDL_PIXELFORMAT_ARGB8888)};
    if (conv == nullptr)
        return;
    int x0 = cx - conv->w / 2;
    int y0 = cy - conv->h / 2;
    for (int y = 0; y < conv->h; ++y)
    {
        int dy = y0 + y;
        if (dy < 0 || dy >= dst->h)
            continue;
        const auto *s = static_cast<const uint8_t *>(conv->pixels) + static_cast<size_t>(y) * conv->pitch;
        auto *d = static_cast<uint8_t *>(dst->pixels) + static_cast<size_t>(dy) * dst->pitch;
        for (int x = 0; x < conv->w; ++x)
        {
            int dx = x0 + x;
            if (dx < 0 || dx >= dst->w)
                continue;
            const uint8_t *sp = s + static_cast<size_t>(x) * 4; // B,G,R,A
            uint8_t *dp = d + static_cast<size_t>(dx) * 4;
            uint8_t a = sp[3];
            if (a == 0)
                continue;
            dp[0] = static_cast<uint16_t>(sp[0]) * a / 255 + dp[0] * (255 - a) / 255;
            dp[1] = static_cast<uint16_t>(sp[1]) * a / 255 + dp[1] * (255 - a) / 255;
            dp[2] = static_cast<uint16_t>(sp[2]) * a / 255 + dp[2] * (255 - a) / 255;
        }
    }
}

}

SDL_Texture *PaintApp::buildMenuTexture()
{
    SurfacePtr surf{SDL_CreateSurface(SCREEN_WIDTH, MENU_HEIGHT, SDL_PIXELFORMAT_ARGB8888)};
    if (surf == nullptr)
        return nullptr;
    SDL_memset(surf->pixels, 255, static_cast<size_t>(surf->pitch) * surf->h);

    for (const ToolSlot &slot : toolSlots)
    {
        SurfacePtr icon{SDL_LoadBMP(assetPath(slot.icon).c_str())};
        if (icon == nullptr)
            continue;
        SDL_Rect r = toolSlotRect(slot);
        compositeIcon(surf.get(), icon, r.x + r.w / 2, r.y + r.h / 2);
    }

    for (int i = 0; i < NUM_COLORS; ++i)
    {
        SDL_Rect rect = menuColorRect(i);
        const Color &c = colors[menuColorOrder[i]];
        uint32_t px = SDL_MapSurfaceRGBA(surf.get(), c.r, c.g, c.b, 255);
        auto *dst = static_cast<uint32_t *>(surf->pixels);
        for (int y = rect.y; y < rect.y + rect.h; ++y)
            for (int x = rect.x; x < rect.x + rect.w; ++x)
                dst[static_cast<size_t>(y) * surf->w + x] = px;
    }

    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, surf.get());
    return t;
}

void PaintApp::clearScreen()
{
    points.clear();
    color = colors[BLACK];
    drawing = false;
    tool = Tool::Pencil;
    thickness = 2;
    shapeStart = {-1, -1};
    camX = 0;
    camY = 0;
    zoom = 1.0f;
    scrollDrag = ScrollDrag::None;
    canvas.clear();
    dirty = true;
    setCursorForTool();
}

void PaintApp::run()
{
    clearScreen();
    lastMouseX = SCREEN_WIDTH / 2;
    lastMouseY = MENU_HEIGHT + (SCREEN_HEIGHT - MENU_HEIGHT) / 2;
    while (!quit)
    {
        handleInput();
        ensureCanvasCoversView();
        drawScreen();
        SDL_Delay(FRAME_DELAY_MS);
    }
}

void PaintApp::smokeTest()
{
    clearScreen();
    drawLine(100, 200, 600, 400);
    drawCircle(640, 360, 800, 360);
    drawScreen();
    SDL_Delay(100);

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

void PaintApp::openSaveDialog()
{
    static const SDL_DialogFileFilter bmpFilter[] = {{"Bitmap image", "bmp"}};
    SDL_ShowSaveFileDialog(&PaintApp::onSaveDialogDone, this, window, bmpFilter, 1, nullptr);
}

void SDLCALL PaintApp::onSaveDialogDone(void *userdata, const char *const *filelist,
                                        int filter)
{
    (void)filter;
    auto *app = static_cast<PaintApp *>(userdata);
    std::string chosen;
    if (filelist != nullptr && filelist[0] != nullptr)
        chosen = filelist[0];
    std::lock_guard<std::mutex> lock(app->saveDialogMutex);
    app->saveDialogResult = std::move(chosen);
    app->saveDialogDone = true;
}

void PaintApp::processSaveDialog()
{
    std::string path;
    {
        std::lock_guard<std::mutex> lock(saveDialogMutex);
        if (!saveDialogDone)
            return;
        saveDialogDone = false;
        path = std::move(saveDialogResult);
        saveDialogResult.clear();
    }
    if (path.empty()) // cancelled or no file picked: abort quietly
        return;

    if (access(path.c_str(), F_OK) == 0 && !confirmOverwrite(path))
        return;
    writeTo(path);
}

bool PaintApp::confirmOverwrite(const std::string &path) const
{
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
         0, "Cancel"},
        {0, 1, "Overwrite"},
    };
    const SDL_MessageBoxData box{SDL_MESSAGEBOX_WARNING,
                                 window,
                                 "Overwrite existing file?",
                                 path.c_str(),
                                 2,
                                 buttons,
                                 nullptr};
    int button = 0;
    if (!SDL_ShowMessageBox(&box, &button))
        return false;
    return button == 1;
}

void PaintApp::buildHelpTexture()
{
#ifdef HAS_SDL_TTF
    static constexpr std::array<std::string_view, 9> fontCandidates = {
        "/usr/share/fonts/liberation-sans-fonts/LiberationMono-Regular.ttf",
        "/usr/share/fonts/google-noto/NotoSansMono[wght].ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    for (std::string_view path : fontCandidates)
    {
        if (access(path.data(), R_OK) != 0)
            continue;
        helpFont = TTF_OpenFont(path.data(), HELP_FONT_SIZE);
        if (helpFont != nullptr)
            break;
    }
    if (helpFont == nullptr)
    {
        std::cerr << "Help popup disabled: no usable font found" << std::endl;
        return;
    }

    TTF_SetFontStyle(helpFont, TTF_STYLE_BOLD);

    static constexpr std::array lines = {
        "DRAWING",
        "  Left-click and drag to draw with pencil or eraser.",
        "  Shapes (line, circle, rect): click start point, then end point.",
        "  Fill (bucket): click inside a region to fill it.",
        "",
        "NAVIGATION",
        "  Mouse wheel: scroll up/down.",
        "  Shift+wheel: scroll left/right.",
        "  Middle-mouse drag: pan.  Arrow keys: pan.",
        "  Scroll bars: drag the bars on the right/bottom edges.",
        "",
        "TOOLS & COLORS",
        "  [1-7]      Select color",
        "  [Z]/[X]    Thickness +/-",
        "  [F]        Fill tool",
        "  [C]        Clear canvas",
        "",
        "FILE",
        "  [Ctrl+S]   Save as BMP",
        "  [Ctrl+G]   Toggle grid",
        "  [Ctrl+/]   Show this help",
    };

    const SDL_Color textColor{HELP_BODY_TEXT.r, HELP_BODY_TEXT.g, HELP_BODY_TEXT.b, 255};
    const SDL_Color titleColor{HELP_TITLE_TEXT.r, HELP_TITLE_TEXT.g, HELP_TITLE_TEXT.b, 255};

    std::vector<SDL_Surface *> lineSurfs;
    lineSurfs.reserve(lines.size());
    int bodyW = 0, bodyH = 0;
    for (const char *text : lines)
    {
        SDL_Surface *s = TTF_RenderText_Blended(helpFont, text, std::strlen(text), textColor);
        if (s == nullptr)
            continue;
        bodyW = std::max(bodyW, s->w);
        bodyH += s->h + HELP_LINE_SPACING;
        lineSurfs.push_back(s);
    }
    bodyH -= HELP_LINE_SPACING;

    SurfacePtr body{SDL_CreateSurface(bodyW, bodyH, SDL_PIXELFORMAT_ARGB8888)};
    if (body != nullptr)
    {
        SDL_memset(body->pixels, 0, static_cast<size_t>(body->pitch) * body->h); // transparent
        int y = 0;
        for (SDL_Surface *s : lineSurfs)
        {
            SDL_Rect dst{0, y, s->w, s->h};
            SDL_BlitSurface(s, nullptr, body.get(), &dst);
            y += s->h + HELP_LINE_SPACING;
        }
    }
    for (SDL_Surface *s : lineSurfs)
        SDL_DestroySurface(s);

    helpTex = body ? SDL_CreateTextureFromSurface(renderer, body.get()) : nullptr;

    TTF_SetFontSize(helpFont, HELP_TITLE_FONT_SIZE);
    const char *helpTitle = "Paint - Help";
    SDL_Surface *title =
        TTF_RenderText_Blended(helpFont, helpTitle, std::strlen(helpTitle), titleColor);
    if (title != nullptr)
    {
        helpTitleTex = SDL_CreateTextureFromSurface(renderer, title);
        SDL_DestroySurface(title);
    }
    TTF_SetFontSize(helpFont, HELP_FONT_SIZE);

    float titleW = 0, titleH = 0;
    if (helpTitleTex != nullptr)
        SDL_GetTextureSize(helpTitleTex, &titleW, &titleH);

    int panelW = std::max(std::max(bodyW, static_cast<int>(titleW)) + 2 * HELP_PAD,
                          HELP_MIN_PANEL_W);
    int panelH = std::clamp(HELP_HEADER_H + bodyH + 2 * HELP_PAD, 0, SCREEN_HEIGHT - HELP_MAX_H_MARGIN);
    helpPanel = {(SCREEN_WIDTH - panelW) / 2, (SCREEN_HEIGHT - panelH) / 2, panelW, panelH};
    helpClose = {helpPanel.x + helpPanel.w - HELP_CLOSE_MARGIN_R,
                 helpPanel.y + HELP_CLOSE_MARGIN_T, HELP_CLOSE_SIZE, HELP_CLOSE_SIZE};
#endif
}

void PaintApp::drawHelpPopup()
{
    if (!helpOpen || helpTex == nullptr)
        return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, HELP_DIM.r, HELP_DIM.g, HELP_DIM.b, HELP_DIM_ALPHA);
    SDL_RenderFillRect(renderer, nullptr);

    SDL_SetRenderDrawColor(renderer, HELP_PANEL_BG.r, HELP_PANEL_BG.g, HELP_PANEL_BG.b, 255);
    SDL_FRect panel{static_cast<float>(helpPanel.x), static_cast<float>(helpPanel.y),
                    static_cast<float>(helpPanel.w), static_cast<float>(helpPanel.h)};
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, HELP_PANEL_BORDER.r, HELP_PANEL_BORDER.g,
                           HELP_PANEL_BORDER.b, 255);
    SDL_RenderRect(renderer, &panel);

    if (helpTitleTex != nullptr)
    {
        float w = 0, h = 0;
        SDL_GetTextureSize(helpTitleTex, &w, &h);
        SDL_FRect dst{static_cast<float>(helpPanel.x + HELP_PAD),
                      static_cast<float>(helpPanel.y + HELP_TITLE_OFFSET_Y), w, h};
        SDL_RenderTexture(renderer, helpTitleTex, nullptr, &dst);
    }
    if (helpTex != nullptr)
    {
        float w = 0, h = 0;
        SDL_GetTextureSize(helpTex, &w, &h);
        SDL_FRect dst{static_cast<float>(helpPanel.x + HELP_PAD),
                      static_cast<float>(helpPanel.y + HELP_HEADER_H), w, h};
        SDL_RenderTexture(renderer, helpTex, nullptr, &dst);
    }

    SDL_SetRenderDrawColor(renderer, HELP_CLOSE_BG.r, HELP_CLOSE_BG.g, HELP_CLOSE_BG.b, 255);
    SDL_FRect closeBox{static_cast<float>(helpClose.x), static_cast<float>(helpClose.y),
                       static_cast<float>(helpClose.w), static_cast<float>(helpClose.h)};
    SDL_RenderFillRect(renderer, &closeBox);
    SDL_SetRenderDrawColor(renderer, HELP_CLOSE_MARK.r, HELP_CLOSE_MARK.g, HELP_CLOSE_MARK.b, 255);
    const int inset = HELP_CLOSE_CROSS_INSET;
    const int farX = helpClose.w - inset;
    const int farY = helpClose.h - inset;
    SDL_RenderLine(renderer, static_cast<float>(helpClose.x + inset),
                   static_cast<float>(helpClose.y + inset),
                   static_cast<float>(helpClose.x + farX),
                   static_cast<float>(helpClose.y + farY));
    SDL_RenderLine(renderer, static_cast<float>(helpClose.x + farX),
                   static_cast<float>(helpClose.y + inset),
                   static_cast<float>(helpClose.x + inset),
                   static_cast<float>(helpClose.y + farY));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

}
