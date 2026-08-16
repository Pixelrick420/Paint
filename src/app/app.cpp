#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "app.hpp"

namespace paint
{

PaintApp::PaintApp()
    : window(nullptr), renderer(nullptr), canvasTex(nullptr),
      menuTex(nullptr), fillTex(nullptr), fillSurf(nullptr)
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

    fillSurf = SDL_LoadBMP(assetPath("fill.bmp").c_str());
    if (fillSurf != nullptr)
    {
        fillTex = SDL_CreateTextureFromSurface(renderer, fillSurf);
        if (fillTex != nullptr)
        {
            SDL_SetTextureBlendMode(fillTex, SDL_BLENDMODE_BLEND);
            fillTexW = fillSurf->w;
            fillTexH = fillSurf->h;
        }
    }
}

PaintApp::~PaintApp()
{
    SDL_DestroyTexture(fillTex);
    SDL_FreeSurface(fillSurf);
    SDL_DestroyTexture(menuTex);
    SDL_DestroyTexture(canvasTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Texture *PaintApp::buildMenuTexture()
{
    std::ifstream file(assetPath("Menu.bin"), std::ios::binary);
    if (!file)
        return nullptr;
    std::vector<uint8_t> pixels(SCREEN_WIDTH * MENU_HEIGHT);
    if (!file.read((char *)pixels.data(), pixels.size()))
        return nullptr;

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, SCREEN_WIDTH, MENU_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf)
        return nullptr;
    for (int i = 0; i < MENU_HEIGHT; ++i)
    {
        for (int j = 0; j < SCREEN_WIDTH; ++j)
        {
            const auto &c = colors[(pixels[i * SCREEN_WIDTH + j] >> 5) & 0x07];
            ((uint32_t *)surf->pixels)[i * SCREEN_WIDTH + j] =
                SDL_MapRGBA(surf->format, c.r, c.g, c.b, 255);
        }
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

void PaintApp::printHelp()
{
    std::cout << R"(
================== Paint Program Help ==================
Controls:
1. Mouse:
   - Click a tool or a color in the menu to select it.
   - Left-click and drag to draw with the pencil tool.
   - Use the eraser tool to erase parts of the drawing.
   - For the line, circle, and rectangle tools, click once
     for the start and once for the end of the shape; a
     preview follows your mouse between the two clicks.
   - Fill (bucket) tool: click inside a region to flood-fill it.
   - Ctrl + mouse wheel: zoom in/out at the cursor.
   - Mouse wheel: scroll up/down.
   - Shift + mouse wheel: scroll left/right.
   - Drag the scroll bars (right and bottom edges) to scroll the canvas.
   - Middle-mouse drag or arrow keys: pan the canvas.

2. Keyboard:
   - [1-7]: Select a color.
   - [C]: Clear the canvas.
   - [Z]: Increase the line thickness.
   - [X]: Decrease the line thickness.
   - [F]: Select the fill (bucket) tool.
   - [Ctrl+S]: Save the drawing as a BMP in the current directory.
   - [Ctrl+G]: Toggle the grid.

3. Tools in the Menu:
   First row:
       1. Pencil tool
       2. Line tool
       3. Rectangle tool
       4. Black color
       5. Blue color
       6. Yellow color
       7. Gray color
       8. Fill tool
   Second row:
       1. Eraser
       2. Circle tool
       3. Help
       4. Purple color
       5. Green color
       6. Red color
       7. White color

Instructions:
1. Select a tool or a color from the menu.
2. Use the left mouse button on the canvas.
3. Change colors or tools anytime by clicking menu icons.
4. Use keyboard shortcuts for quicker actions.

The canvas grows in any direction as you move toward its
edges, up to a maximum size, after which new cells are no
longer generated.

    Have fun creating your masterpiece!
====================================================
)" << std::endl;
}

}

