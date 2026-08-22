# Paint

Paint is a simple drawing program for Linux. It uses the SDL3 and SDL3_ttf libraries.

## Requirements

- Linux
- A C++ compiler that supports C++23
- SDL3 development files
- SDL3_ttf development files
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
- Use the scroll wheel to zoom. Drag the scroll bars to pan.

Use the keyboard:

- Press `1` to `7` to select a color.
- Press `c` to clear the screen.
- Press `z` or `x` to change the line thickness.
- Press `f` to select the fill tool.
- Press `Ctrl+s` to save the drawing as a BMP file. The program opens a file chooser for the folder and the file name.
- Press `Ctrl+g` to toggle the grid.
- Press `Ctrl+/` to open or close the help popup.
- Press `Esc` to close the help popup.
- Use the arrow keys to pan.

## Menu tools

First row: pencil, line, rectangle, black, blue, yellow, gray.

Second row: eraser, circle, help, purple, green, red, white.

## Smoke test

Run `PAINT_SMOKE_TEST=1 ./paint` to test the drawing functions. The program exits when the test completes.

## Static build

The GitHub Actions workflow builds a static binary. This binary runs on Linux systems without SDL3, SDL3_ttf, or the C++ runtime.
