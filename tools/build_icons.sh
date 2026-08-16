#!/usr/bin/env bash
# Regenerates the menu tool icons and mouse cursor bitmaps from the PNG
# sources in assets/icons/ into 32-bit BMPs (with alpha) that SDL_LoadBMP
# reads as ARGB8888.
#
# Icons: tstamborski/pixelart-icons (CC0 / public domain)
#   https://github.com/tstamborski/pixelart-icons
#   pencil16, paint-bucket16, rubber16  (16px, scaled 2x to match the
#   32px menu grid and keep the pixel-art look chunky)
# line.png, square-outline.png and circle-outline.png are generated
# locally (2px black strokes, same style as the line icon - no outline
# equivalents exist in the set). crosshair.png is also local.
#
# Requires ImageMagick (magick). Idempotent; run from the repo root.
set -euo pipefail
cd "$(dirname "$0")/.."

magick assets/icons/pencil16.png -sample 200% assets/tool_pencil.bmp
magick assets/icons/line.png            assets/tool_line.bmp
magick assets/icons/square-outline.png  assets/tool_rectangle.bmp
magick assets/icons/paint-bucket16.png -sample 200% assets/tool_fill.bmp
magick assets/icons/rubber16.png  -sample 200% assets/tool_eraser.bmp
magick assets/icons/circle-outline.png  assets/tool_circle.bmp

magick assets/icons/pencil32.png        assets/pencil.bmp
magick assets/icons/crosshair.png       assets/point.bmp
magick assets/icons/rubber16.png        assets/eraser.bmp

echo "icons rebuilt: $(ls assets/tool_*.bmp assets/pencil.bmp assets/point.bmp assets/eraser.bmp | wc -l) files"
