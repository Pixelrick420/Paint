#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>

struct Color
{
    uint8_t r, g, b;
};

struct Slider
{
    int x, y;
    int value;
};

class PaintApp
{
private:
    static constexpr int NUM_COLORS = 8;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr int SCREEN_WIDTH = 1280;
    static constexpr int POINT_THRESHOLD = 500;
    static constexpr int MENU_HEIGHT = 112;
    static constexpr int MENU_WIDTH = 350;
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
    std::vector<SDL_Point> points[NUM_COLORS];

    int startX{0}, startY{0}, endX{0}, endY{0};
    int color{0};
    int mode{0}; // 0=idle, 1=drawing, 2=erasing, 3=line, 4=circle, 5=rectangle
    int thickness{2};
    bool quit{false};

    void drawPoint(int x, int y)
    {
        if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        {
            points[color].push_back({x, y});
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

    void setCursor(int type)
    {
        const char *cursorFile = nullptr;
        switch (type)
        {
        case 0:
            cursorFile = "pencil.bmp";
            break;
        case 1:
            cursorFile = "point.bmp";
            break;
        case 2:
            cursorFile = "eraser.bmp";
            break;
        default:
            SDL_SetCursor(SDL_GetDefaultCursor());
            return;
        }

        if (SDL_Surface *cursorSurface = SDL_LoadBMP(cursorFile))
        {
            if (SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, cursorSurface->h - 1))
            {
                SDL_SetCursor(cursor);
            }
            SDL_FreeSurface(cursorSurface);
        }
    }

    void handleToolSelection(int row, int col)
    {
        if (row == 0)
        {
            switch (col)
            {
            case 0:
                mode = 1;
                setCursor(0);
                break; // Pencil
            case 1:
                mode = 3;
                setCursor(1);
                break; // Line
            case 2:
                mode = 5;
                setCursor(1);
                break; // Rectangle
            case 3:
                color = 0;
                break;
            case 4:
                color = 2;
                break;
            case 5:
                color = 4;
                break;
            case 6:
                color = 6;
                break;
            }
        }
        else if (row == 1)
        {
            switch (col)
            {
            case 0:
                mode = 2;
                setCursor(3);
                break; // Eraser
            case 1:
                mode = 4;
                setCursor(1);
                break; // Circle
            case 2:
                mode = 0;
                printHelp();
                break; // Help
            case 3:
                color = 1;
                break;
            case 4:
                color = 3;
                break;
            case 5:
                color = 5;
                break;
            case 6:
                color = 7;
                break;
            }
        }
    }

    void drawOverlay()
    {
        std::ifstream file("Menu.bin", std::ios::binary);
        std::vector<uint8_t> pixels(MENU_HEIGHT * SCREEN_WIDTH);

        if (file.read(reinterpret_cast<char *>(pixels.data()), pixels.size()))
        {
            for (int i = 0; i < MENU_HEIGHT; ++i)
            {
                for (int j = 0; j < SCREEN_WIDTH; ++j)
                {
                    uint8_t value = pixels[i * SCREEN_WIDTH + j];
                    int pixel = (value >> 5) & 0x07;
                    const auto &col = colors[pixel];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_RenderDrawPoint(renderer, j, i);
                }
            }
        }
    }

public:
    PaintApp() : window(nullptr), renderer(nullptr)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Paint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }

    ~PaintApp()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void clearScreen()
    {
        for (auto &pointSet : points)
        {
            pointSet.clear();
        }
        color = 0;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        drawOverlay();
    }

    void drawScreen()
    {
        for (int i = 0; i < NUM_COLORS; ++i)
        {
            if (!points[i].empty())
            {
                const auto &col = colors[i];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                size_t startIdx = points[i].size() > POINT_THRESHOLD ? points[i].size() - POINT_THRESHOLD : 0;
                for (size_t j = startIdx; j < points[i].size(); ++j)
                {
                    SDL_RenderDrawPoint(renderer, points[i][j].x, points[i][j].y);
                }
            }
        }
        SDL_RenderPresent(renderer);
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
                startX = e.motion.x;
                startY = e.motion.y;
                if (startY < MENU_HEIGHT)
                {
                    handleToolSelection(startY / ROW_HEIGHT, startX / TOOL_WIDTH);
                }
                else
                {
                    switch (mode)
                    {
                    case 0: // pencil
                        mode = 1;
                        drawPoint(startX, startY);
                        break;
                    case 1: // eraser
                        mode = 1;
                        color = 7;
                        drawPoint(startX, startY);
                        break;
                    case 2:
                        break;
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                mode = 0;
                break;

            case SDL_MOUSEMOTION:
                if (mode == 1 && startY >= MENU_HEIGHT + 20)
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
                    color = e.key.keysym.sym - SDLK_1;
                }
                else if (e.key.keysym.sym == SDLK_c)
                {
                    clearScreen();
                }
                else if (e.key.keysym.sym == SDLK_d)
                {
                    mode = 1;
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

int main(int argc, char **argv)
{
    PaintApp app;
    app.run();
    return 0;
}