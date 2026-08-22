#pragma once

#include <cstdint>

namespace paint
{

// Window --------------------------------------------------------------------
inline constexpr int SCREEN_WIDTH = 1280;
inline constexpr int SCREEN_HEIGHT = 720;

// Menu bar (tools on the left, colors on the right) --------------------------
inline constexpr int MENU_HEIGHT = 112;
inline constexpr int TOOL_WIDTH = 50;
inline constexpr int ROW_HEIGHT = 56;
inline constexpr int NUM_COLORS = 8;
inline constexpr int MENU_COLORS_PER_ROW = NUM_COLORS / 2;
inline constexpr int MENU_COLOR_LEFT =
    SCREEN_WIDTH - MENU_COLORS_PER_ROW * TOOL_WIDTH; // right-aligned block

// Status strip between menu and canvas ---------------------------------------
inline constexpr int STATUS_STRIP_H = 20;

// Thickness preview bar in the status strip ----------------------------------
inline constexpr int THICKNESS_BAR_X = 12;
inline constexpr int THICKNESS_BAR_H = 8;
inline constexpr int THICKNESS_BAR_MIN_LEN = 8;
inline constexpr int THICKNESS_BAR_MAX_LEN = 380;
inline constexpr int THICKNESS_BAR_SCALE = 4; // screen px per thickness step

// Growing canvas ---------------------------------------------------------------
inline constexpr int MAX_GRID = 4096; // hard cap: max cells per side
inline constexpr int INITIAL_CANVAS_WIDTH = SCREEN_WIDTH;
inline constexpr int INITIAL_CANVAS_HEIGHT = SCREEN_HEIGHT - MENU_HEIGHT;
inline constexpr int CHUNK = 64; // canvas growth step (cells)

// Camera / zoom -----------------------------------------------------------------
inline constexpr float MIN_ZOOM = 0.1f;
inline constexpr float MAX_ZOOM = 32.0f;
inline constexpr float ZOOM_STEP = 1.1f;     // multiplicative step per ctrl+wheel notch
inline constexpr float SCROLL_PAN = 60.0f;   // world pan per wheel notch
inline constexpr float ARROW_PAN_PX = 40.0f; // screen px panned per arrow-key press
inline constexpr int MIN_GRID_PX = 10;       // min on-screen grid spacing

// Drawing ------------------------------------------------------------------------
inline constexpr int MAX_LINE_THICKNESS = 64;
inline constexpr int POINT_THRESHOLD = 100000; // flush pending stroke after this many points
inline constexpr int FRAME_DELAY_MS = 16;      // ~60 fps main-loop cap

// Scroll bars ----------------------------------------------------------------------
inline constexpr int SCROLLBAR_W = 12;   // scroll bar thickness (px)
inline constexpr int MIN_THUMB_LEN = 24; // minimum scroll bar thumb length

// Save flash overlay ("Saved" badge, top-right of the menu) -------------------------
inline constexpr int SAVE_FLASH_MS = 1200;
inline constexpr int SAVE_FLASH_MARGIN_R = 96; // from right edge to rect left
inline constexpr int SAVE_FLASH_MARGIN_T = 14; // below the menu
inline constexpr int SAVE_FLASH_W = 84;
inline constexpr int SAVE_FLASH_H = 22;

// Help popup -------------------------------------------------------------------------
inline constexpr int HELP_FONT_SIZE = 18;
inline constexpr int HELP_TITLE_FONT_SIZE = 28;
inline constexpr int HELP_PAD = 20;         // inner padding + title/body x offset
inline constexpr int HELP_TITLE_OFFSET_Y = 14;
inline constexpr int HELP_HEADER_H = 56;    // header height + body y offset
inline constexpr int HELP_LINE_SPACING = 4; // gap between rendered text lines
inline constexpr int HELP_MIN_PANEL_W = 600;
inline constexpr int HELP_MAX_H_MARGIN = 40; // panel never grows into this margin
inline constexpr int HELP_CLOSE_SIZE = 26;
inline constexpr int HELP_CLOSE_MARGIN_R = 34; // from panel right edge to button left
inline constexpr int HELP_CLOSE_MARGIN_T = 8;  // from panel top to button top
inline constexpr int HELP_CLOSE_CROSS_INSET = 7;

// Mouse cursors ------------------------------------------------------------------------
inline constexpr int CURSOR_MIN_SIZE = 24;
inline constexpr int CURSOR_THICKNESS_SCALE = 3; // eraser cursor size = thickness * this

// UI palette ---------------------------------------------------------------------------
struct Color
{
    uint8_t r, g, b;
};

inline constexpr uint8_t CANVAS_BG = 205; // grey backdrop around the canvas texture

inline constexpr Color GRID_LINE{235, 235, 235};
inline constexpr Color GRID_LINE_5X{200, 200, 200};  // every 5th cell
inline constexpr Color GRID_LINE_10X{140, 140, 140}; // every 10th cell

inline constexpr Color STATUS_BG{238, 238, 238};
inline constexpr Color STATUS_EDGE{180, 180, 180}; // line under the menu
inline constexpr Color STATUS_SEP{80, 80, 80};     // line under the strip

inline constexpr Color SCROLL_TRACK{218, 218, 218};
inline constexpr Color SCROLL_THUMB{158, 158, 158};

inline constexpr Color FLASH_BG{60, 200, 90};
inline constexpr uint8_t FLASH_ALPHA = 130;

inline constexpr Color HELP_DIM{0, 0, 0};
inline constexpr uint8_t HELP_DIM_ALPHA = 160;
inline constexpr Color HELP_PANEL_BG{30, 30, 46};
inline constexpr Color HELP_PANEL_BORDER{140, 140, 160};
inline constexpr Color HELP_CLOSE_BG{70, 70, 92};
inline constexpr Color HELP_CLOSE_MARK{230, 230, 230}; // the X glyph
inline constexpr Color HELP_TITLE_TEXT{255, 255, 255};
inline constexpr Color HELP_BODY_TEXT{230, 230, 230};

}

