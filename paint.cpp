#include <SDL3/SDL_main.h>

#include <cstdlib>

#include "app/app.hpp"

int main()
{
    paint::PaintApp app;
    if (std::getenv("PAINT_SMOKE_TEST") != nullptr)
    {
        app.smokeTest();
        return 0;
    }
    app.run();
    return 0;
}
