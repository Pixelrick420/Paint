# Paint

Paint is a simple drawing program for Linux. It uses the SDL2 library.

## Requirements

- Linux
- A C++ compiler that supports C++23
- SDL2 development files
- pkg-config
- Make

## Build

Run `make` in the project directory.

## Run

Run `./paint` in the project directory.

## Controls

Use the mouse:

- Click a tool or a color in the menu to select it.
- Drag on the canvas to draw with the pencil.
- Drag on the canvas to erase with the eraser.
- For the line, circle, and rectangle tools, click once for the start and once for the end of the shape.

Use the keyboard:

- Press `1` to `7` to select a color.
- Press `c` to clear the screen.
- Press `z` to increase the line thickness.
- Press `x` to decrease the line thickness.

## Menu tools

First row: pencil, line, rectangle, black, blue, yellow, gray.

Second row: eraser, circle, help, purple, green, red, white.

## Smoke test

Run `PAINT_SMOKE_TEST=1 ./paint` to test the drawing functions. The program exits when the test completes.

## Static build

The GitHub Actions workflow builds a static binary. The static binary runs on Linux without SDL2 or the C++ runtime installed.
