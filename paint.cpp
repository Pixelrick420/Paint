#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <deque>
#include <cmath>
#include <unistd.h>
#include <cstdlib>

static std::string assetPath(const std::string &name)
{
    std::string base = "assets";
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0)
    {
        buf[len] = '\0';
        std::string exePath(buf);
        std::string::size_type slash = exePath.find_last_of('/');
        if (slash != std::string::npos)
            base = exePath.substr(0, slash) + "/assets";
    }
    return base + "/" + name;
}

struct Color
{
    uint8_t r, g, b;
};

struct Slider
{
    int x, y, value;
};

class PaintApp
{
private:
    static constexpr int NUM_COLORS = 8;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int POINT_THRESHOLD = 100000;
    static constexpr int MENU_HEIGHT = 112;
    static constexpr int TOOL_WIDTH = 50;
    static constexpr int ROW_HEIGHT = 56;

    const std::array<Color, NUM_COLORS> colors = {{
        {0, 0, 0},       // black
        {163, 73, 164},  // purple
        {63, 72, 204},   // blue
        {34, 177, 76},   // green
        {255, 201, 14},  // yellow
        {237, 28, 36},   // red
        {127, 127, 127}, // gray
        {255, 255, 255}  // white
    }};

    SDL_Window *window;
    SDL_Renderer *renderer;
    std::deque<SDL_Point> points;
    std::vector<Slider> sliders = {
        {8, 961, 0},  // r
        {24, 961, 0}, // g
        {40, 961, 0}, // b
        {76, 961, 0}  // size
    };

    int startX{0}, startY{0}, endX{0}, endY{0};
    Color color{0, 0, 0};
    int mode{0}; // 0 = not clicked, 1 = using tool
    int tool{1}; // Current tool: 1=pencil, 2=eraser, 3=line, 4=circle, 5=rectangle
    int thickness{2};
    bool quit{false};
    SDL_Point shapeStart = {-1, -1};

    void drawPoint(int x, int y)
    {
        points.push_back({x, y});
        if (points.size() > POINT_THRESHOLD)
        {
            points.pop_front();
        }
    }

    void drawLine(int x1, int y1, int x2, int y2)
    {
        int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true)
        {
            for (int tx = -thickness; tx <= thickness; ++tx)
            {
                for (int ty = -thickness; ty <= thickness; ++ty)
                {
                    drawPoint(x1 + tx, y1 + ty);
                }
            }

            if (x1 == x2 && y1 == y2)
                break;

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y1 += sy;
            }
        }
    }

    void drawCircle(int cx, int cy, int x, int y)
    {
        int radius = (int)std::sqrt((cx - x) * (cx - x) + (cy - y) * (cy - y));
        int radiusSq = radius * radius, circleThickness = thickness * 2;
        for (int dx = -radius - circleThickness; dx <= radius + circleThickness; dx++)
        {
            for (int dy = -radius - circleThickness; dy <= radius + circleThickness; dy++)
            {
                int curx = cx + dx;
                int cury = cy + dy;

                if (curx >= 0 && curx < SCREEN_WIDTH &&
                    cury >= MENU_HEIGHT + 20 && cury < SCREEN_HEIGHT)
                {
                    int distSq = dx * dx + dy * dy;
                    if (abs(distSq - radiusSq) <= circleThickness * radius)
                    {
                        drawPoint(curx, cury);
                    }
                }
            }
        }
    }

    void drawRectangle(int x1, int y1, int x2, int y2)
    {
        int left = std::min(x1, x2);
        int right = std::max(x1, x2);
        int top = std::min(y1, y2);
        int bottom = std::max(y1, y2);

        for (int x = left - thickness; x <= right + thickness; x++)
        {
            if (x >= 0 && x < SCREEN_WIDTH)
            {
                for (int t = -thickness; t <= thickness; t++)
                {
                    int y1_t = top + t;
                    int y2_t = bottom + t;
                    if (y1_t >= MENU_HEIGHT + 20 && y1_t < SCREEN_HEIGHT)
                        drawPoint(x, y1_t);
                    if (y2_t >= MENU_HEIGHT + 20 && y2_t < SCREEN_HEIGHT)
                        drawPoint(x, y2_t);
                }
            }
        }

        for (int y = top - thickness; y <= bottom + thickness; y++)
        {
            if (y >= MENU_HEIGHT + 20 && y < SCREEN_HEIGHT)
            {
                for (int t = -thickness; t <= thickness; t++)
                {
                    int x1_t = left + t;
                    int x2_t = right + t;
                    if (x1_t >= 0 && x1_t < SCREEN_WIDTH)
                        drawPoint(x1_t, y);
                    if (x2_t >= 0 && x2_t < SCREEN_WIDTH)
                        drawPoint(x2_t, y);
                }
            }
        }
    }

    void setCursor(int type)
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
        int cursorSize = thickness * 3;
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

    void handleToolSelection(int row, int col)
    {
        if (row == 0)
        {
            switch (col)
            {
            case 0:
                tool = 1;
                color = colors[0];
                setCursor(0);
                break; // Pencil
            case 1:
                tool = 3;
                setCursor(1);
                break; // Line
            case 2:
                tool = 5;
                setCursor(1);
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
            }
        }
        else if (row == 1)
        {
            switch (col)
            {
            case 0:
                tool = 2;
                color = colors[7];
                setCursor(2);
                break; // Eraser
            case 1:
                tool = 4;
                setCursor(1);
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

    void drawPalettePixels(const std::vector<uint8_t> &pixels, int width, int height, int offsetX, int offsetY)
    {
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                uint8_t value = pixels[i * width + j];
                const auto &col = colors[(value >> 5) & 0x07];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_RenderDrawPoint(renderer, j + offsetX, i + offsetY);
            }
        }
    }

    void drawSliders()
    {
        std::ifstream file(assetPath("Slider.bin"), std::ios::binary);
        int width = 16, height = 15;
        std::vector<uint8_t> pixels(width * height);

        if (file.read(reinterpret_cast<char *>(pixels.data()), pixels.size()))
        {
            for (const Slider &slider : sliders)
            {
                drawPalettePixels(pixels, width, height, slider.y, slider.x);
            }
        }
    }

    void drawOverlay()
    {
        std::ifstream file(assetPath("Menu.bin"), std::ios::binary);
        std::vector<uint8_t> pixels(MENU_HEIGHT * SCREEN_WIDTH);

        if (file.read(reinterpret_cast<char *>(pixels.data()), pixels.size()))
        {
            drawPalettePixels(pixels, SCREEN_WIDTH, MENU_HEIGHT, 0, 0);
        }
    }

public:
    PaintApp() : window(nullptr), renderer(nullptr)
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
    }

    ~PaintApp()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void clearScreen()
    {
        points.clear();
        color = {0, 0, 0};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        setCursor(0);
        drawOverlay();
        mode = 0;
        tool = 1;
        thickness = 2;
        shapeStart = {-1, -1};
    }

    void drawScreen()
    {
        if (!points.empty())
        {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            size_t startIdx = points.size() > POINT_THRESHOLD ? points.size() - POINT_THRESHOLD : 0;
            for (size_t i = startIdx; i < points.size(); ++i)
            {
                SDL_RenderDrawPoint(renderer, points[i].x, points[i].y);
            }
        }
        points.clear();
        SDL_RenderPresent(renderer);
    }

    void smokeTest()
    {
        clearScreen();
        drawLine(100, 200, 600, 400);
        drawCircle(640, 360, 800, 360);
        drawScreen();
        SDL_Delay(100);
    }

    void handleInput()
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
                startX = e.button.x;
                startY = e.button.y;
                if (startY < MENU_HEIGHT)
                {
                    handleToolSelection(startY / ROW_HEIGHT, startX / TOOL_WIDTH);
                }
                else
                {
                    mode = 1;
                    drawPoint(startX, startY);
                    if (tool >= 2 && tool <= 5)
                    {
                        if (shapeStart.x != -1)
                        {
                            switch (tool)
                            {
                            case 3:
                                drawLine(shapeStart.x, shapeStart.y, startX, startY);
                                break;
                            case 4:
                                drawCircle(shapeStart.x, shapeStart.y, startX, startY);
                                break;
                            case 5:
                                drawRectangle(shapeStart.x, shapeStart.y, startX, startY);
                                break;
                            }
                            shapeStart = {-1, -1};
                        }
                        else
                        {
                            shapeStart = {startX, startY};
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                mode = 0;
                break;

            case SDL_MOUSEMOTION:
                if (mode == 1 && tool <= 2 && startY >= MENU_HEIGHT + 20)
                {
                    endX = e.motion.x;
                    endY = e.motion.y;
                    if (endY >= MENU_HEIGHT + 20)
                    {
                        drawLine(startX, startY, endX, endY);
                        startX = endX;
                        startY = endY;
                    }
                }
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_7)
                {
                    color = colors[e.key.keysym.sym - SDLK_1];
                }
                switch (e.key.keysym.sym)
                {
                case SDLK_c:
                    clearScreen();
                    break;
                case SDLK_z:
                    thickness++;
                    break;
                case SDLK_x:
                    thickness--;
                    break;
                }
                break;
            }
        }
    }

    void run()
    {
        clearScreen();
        setCursor(0);
        while (!quit)
        {
            handleInput();
            drawScreen();
            SDL_Delay(1);
        }
    }

    static void printHelp()
    {
        std::cout << R"(
================== Paint Program Help ==================
Controls:
1. Mouse:
   - Click on a tool or color in the menu to select it.
   - Left-click and drag to draw using pencil tool.
   - Use the Eraser tool to erase parts of the drawing.

2. Keyboard:
   - [1-7]: Select colors.
   - [D]: Enter drawing mode (Pencil tool).
   - [C]: Clear the screen.

3. Tools in the Menu:
   First row:
       1. Pencil tool (drawing mode)
       2. Line tool
       3. Rectangle tool
       4. Black color
       5. Blue color
       6. Yellow color
       7. Gray color
   Second row:
       1. Eraser
       2. Circle tool
       3. Help
       4. Violet color
       5. Green color
       6. Red color
       7. Clear screen

Instructions:
1. Start by selecting a tool or color from the menu.
2. Use the left mouse button to interact with the canvas.
3. Change colors or tools anytime by clicking menu icons.
4. Use keyboard shortcuts for quicker actions.

Have fun creating your masterpiece!
====================================================
)" << std::endl;
    }
};

int main()
{
    PaintApp app;
    if (std::getenv("PAINT_SMOKE_TEST") != nullptr)
    {
        app.smokeTest();
        return 0;
    }
    app.run();
    return 0;
}
