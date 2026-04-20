#include "touchmap_overlay.h"

#include <assert.h>
#include <math.h>

#include "display.h"
#include "util/log.h"
#include "coords.h"

#define OVERLAY_CIRCLE_POINTS 32

static const int OVERLAY_BUTTON_RADIUS = SC_TOUCHMAP_MIN_RADIUS;
static const int OVERLAY_PRESSED_RADIUS = 10;
static const int OVERLAY_WALK_POS_RADIUS = 5;
static const int OVERLAY_GLYPH_SCALE_BUTTON = 1;
static const int OVERLAY_GLYPH_SCALE_WALK = 3;
static const int OVERLAY_GLYPH_SPACING = -4;
static const int OVERLAY_GLYPH_WIDTH = 24;
static const int OVERLAY_GLYPH_HEIGHT = 24;
static const int OVERLAY_TEXT_SPACING = -8;
static const int OVERLAY_EDIT_PADDING = 6;
static const int OVERLAY_EDIT_MARGIN = 8;
static const int OVERLAY_TOOLBAR_GAP = 6;

/**
 * Helper function to extract RGBA components from a color value
 */
static void
color_to_rgba(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b,
              uint8_t *a) {
    *r = (color >> 24) & 0xFF;
    *g = (color >> 16) & 0xFF;
    *b = (color >> 8) & 0xFF;
    *a = color & 0xFF;
}

/**
 * Draw a filled circle using SDL drawing primitives
 */
static void
draw_filled_circle(SDL_Renderer *renderer, int center_x, int center_y,
                   int radius, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    // Draw filled circle using Bresenham-like algorithm
    for (int y = -radius; y <= radius; ++y) {
        int x = (int)sqrt(radius * radius - y * y);
        SDL_RenderDrawLine(renderer, center_x - x, center_y + y,
                          center_x + x, center_y + y);
    }
}

/**
 * Draw a circle outline (not filled)
 */
static void
draw_circle_outline(SDL_Renderer *renderer, int center_x, int center_y,
                   int radius, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    double angle_step = 2.0 * M_PI / OVERLAY_CIRCLE_POINTS;
    for (int i = 0; i < OVERLAY_CIRCLE_POINTS; ++i) {
        double angle1 = i * angle_step;
        double angle2 = (i + 1) * angle_step;

        int x1 = center_x + (int)(radius * cos(angle1));
        int y1 = center_y + (int)(radius * sin(angle1));
        int x2 = center_x + (int)(radius * cos(angle2));
        int y2 = center_y + (int)(radius * sin(angle2));

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

static void
draw_dashed_circle_outline(SDL_Renderer *renderer, int center_x,
                            int center_y, int radius, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    double angle_step = 2.0 * M_PI / OVERLAY_CIRCLE_POINTS;
    for (int i = 0; i < OVERLAY_CIRCLE_POINTS; ++i) {
        if (i % 2 != 0) {
            continue;
        }
        double angle1 = i * angle_step;
        double angle2 = (i + 1) * angle_step;

        int x1 = center_x + (int)(radius * cos(angle1));
        int y1 = center_y + (int)(radius * sin(angle1));
        int x2 = center_x + (int)(radius * cos(angle2));
        int y2 = center_y + (int)(radius * sin(angle2));

        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}


enum overlay_label {
    OVERLAY_LABEL_NONE,
    OVERLAY_LABEL_A,
    OVERLAY_LABEL_B,
    OVERLAY_LABEL_X,
    OVERLAY_LABEL_Y,
    OVERLAY_LABEL_L1,
    OVERLAY_LABEL_R1,
    OVERLAY_LABEL_L2,
    OVERLAY_LABEL_R2,
    OVERLAY_LABEL_L3,
    OVERLAY_LABEL_R3,
    OVERLAY_LABEL_LEFT,
    OVERLAY_LABEL_RIGHT,
    OVERLAY_LABEL_UP,
    OVERLAY_LABEL_DOWN,
    OVERLAY_LABEL_BACK,
    OVERLAY_LABEL_START,
    OVERLAY_LABEL_GUIDE,
};

/**
 * Get label from SDL button constant
 */
static enum overlay_label
button_value_to_label(uint8_t button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return OVERLAY_LABEL_A;
        case SDL_CONTROLLER_BUTTON_B: return OVERLAY_LABEL_B;
        case SDL_CONTROLLER_BUTTON_X: return OVERLAY_LABEL_X;
        case SDL_CONTROLLER_BUTTON_Y: return OVERLAY_LABEL_Y;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return OVERLAY_LABEL_L3;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return OVERLAY_LABEL_R3;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return OVERLAY_LABEL_L1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return OVERLAY_LABEL_R1;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return OVERLAY_LABEL_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return OVERLAY_LABEL_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return OVERLAY_LABEL_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return OVERLAY_LABEL_RIGHT;
        case SDL_CONTROLLER_BUTTON_BACK: return OVERLAY_LABEL_BACK;
        case SDL_CONTROLLER_BUTTON_START: return OVERLAY_LABEL_START;
        case SDL_CONTROLLER_BUTTON_GUIDE: return OVERLAY_LABEL_GUIDE;
        default:
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                return OVERLAY_LABEL_L2;
            }
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                return OVERLAY_LABEL_R2;
            }
            return OVERLAY_LABEL_NONE;
    }
}

static void
set_overlay_label_color(SDL_Renderer *renderer, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}


enum overlay_glyph {
    OVERLAY_GLYPH_A,
    OVERLAY_GLYPH_B,
    OVERLAY_GLYPH_X,
    OVERLAY_GLYPH_Y,
    OVERLAY_GLYPH_L,
    OVERLAY_GLYPH_R,
    OVERLAY_GLYPH_1,
    OVERLAY_GLYPH_2,
    OVERLAY_GLYPH_3,
    OVERLAY_GLYPH_LEFT,
    OVERLAY_GLYPH_RIGHT,
    OVERLAY_GLYPH_UP,
    OVERLAY_GLYPH_DOWN,
    OVERLAY_GLYPH_BACK,
    OVERLAY_GLYPH_START,
    OVERLAY_GLYPH_GUIDE,
    OVERLAY_GLYPH_C,
    OVERLAY_GLYPH_D,
    OVERLAY_GLYPH_E,
    OVERLAY_GLYPH_I,
    OVERLAY_GLYPH_K,
    OVERLAY_GLYPH_N,
    OVERLAY_GLYPH_O,
    OVERLAY_GLYPH_Q,
    OVERLAY_GLYPH_S,
    OVERLAY_GLYPH_T,
    OVERLAY_GLYPH_U,
    OVERLAY_GLYPH_W,
};

static const uint32_t overlay_glyph_data[][24] = {
    [OVERLAY_GLYPH_A] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x007800, 0x007800,
        0x006C00, 0x00EC00, 0x00CC00, 0x00CE00, 0x00C600, 0x01C600,
        0x018600, 0x018700, 0x038300, 0x03FF00, 0x03FF80, 0x070380,
        0x070180, 0x0601C0, 0x0601C0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_B] = {
        0x000000, 0x000000, 0x000000, 0x01FE00, 0x01FF00, 0x018380,
        0x0181C0, 0x0181C0, 0x0181C0, 0x018180, 0x018380, 0x01FF00,
        0x01FF00, 0x018180, 0x0180C0, 0x0180C0, 0x0180C0, 0x0180C0,
        0x0181C0, 0x01FF80, 0x01FE00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_X] = {
        0x000000, 0x000000, 0x000000, 0x0701C0, 0x030380, 0x018300,
        0x01C700, 0x00C600, 0x00EC00, 0x007C00, 0x007800, 0x003800,
        0x007800, 0x007C00, 0x00EC00, 0x00C600, 0x018700, 0x038300,
        0x030380, 0x070180, 0x0601C0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_Y] = {
        0x000000, 0x000000, 0x000000, 0x0601C0, 0x070180, 0x030380,
        0x038300, 0x018600, 0x00C600, 0x00CC00, 0x007C00, 0x007800,
        0x003800, 0x003000, 0x003000, 0x003000, 0x003000, 0x003000,
        0x003000, 0x003000, 0x003000, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_L] = {
        0x000000, 0x000000, 0x000000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00FFC0, 0x00FFC0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_R] = {
        0x000000, 0x000000, 0x000000, 0x03FC00, 0x03FE00, 0x030700,
        0x030300, 0x030300, 0x030300, 0x030300, 0x030700, 0x03FE00,
        0x03FC00, 0x030E00, 0x030600, 0x030700, 0x030300, 0x030380,
        0x030180, 0x0301C0, 0x0300C0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_1] = {
        0x000000, 0x000000, 0x000000, 0x003C00, 0x00FC00, 0x00CC00,
        0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00,
        0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00,
        0x000C00, 0x00FFC0, 0x00FFC0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_2] = {
        0x000000, 0x000000, 0x000000, 0x007C00, 0x01FF00, 0x018780,
        0x000380, 0x000180, 0x000180, 0x000180, 0x000380, 0x000300,
        0x000600, 0x000C00, 0x001C00, 0x003800, 0x007000, 0x00E000,
        0x01C000, 0x01FF80, 0x01FF80, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_3] = {
        0x000000, 0x000000, 0x000000, 0x007C00, 0x01FF00, 0x018380,
        0x000380, 0x000180, 0x000180, 0x000380, 0x000700, 0x003E00,
        0x003E00, 0x000380, 0x000180, 0x0001C0, 0x0001C0, 0x000180,
        0x010380, 0x01FF00, 0x00FE00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_LEFT] = {
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x030000,
        0x070000, 0x0E0000, 0x1E0000, 0x3FFFFC, 0x3FFFFC, 0x1FFFF8,
        0x0E0000, 0x070000, 0x038000, 0x010000, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_RIGHT] = {
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x0001C0,
        0x0001E0, 0x0000F0, 0x000078, 0x3FFFF8, 0x3FFFFC, 0x1FFFF8,
        0x000070, 0x0000E0, 0x0001C0, 0x000080, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_UP] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x007C00, 0x00FE00,
        0x01FF00, 0x03BB80, 0x013980, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_DOWN] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x03B980, 0x01FB80,
        0x00FF00, 0x007E00, 0x003C00, 0x001800, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_BACK] = {
        0x007E00, 0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC,
        0x7800FE, 0x7800FE, 0x79FFFE, 0xF9FFFF, 0xF9001F, 0xF9001F,
        0xF91F9F, 0xF91F9F, 0xFF1F9F, 0x7F1F9E, 0x7F001E, 0x7F001E,
        0x3FFFFC, 0x1FFFF8, 0x0FFFF0, 0x07FFE0, 0x03FFC0, 0x007E00,
    },
    [OVERLAY_GLYPH_START] = {
        0x007E00, 0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC,
        0x78001E, 0x78001E, 0x7FFFFE, 0xFFFFFF, 0xFFFFFF, 0xF8001F,
        0xF8001F, 0xFFFFFF, 0xFFFFFF, 0x7FFFFE, 0x78001E, 0x78001E,
        0x3FFFFC, 0x1FFFF8, 0x0FFFF0, 0x07FFE0, 0x03FFC0, 0x007E00,
    },
    [OVERLAY_GLYPH_GUIDE] = {
        0x001800, 0x001800, 0x003C00, 0x007E00, 0x00FF00, 0x01FF80,
        0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC, 0x7FFFFE,
        0xFFFFFF, 0x0FFFF0, 0x0FFFF0, 0x0FFFF0, 0x0FC3F0, 0x0FC3F0,
        0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0,
    },
    [OVERLAY_GLYPH_C] = {
        0x000000, 0x000000, 0x000000, 0x001F00, 0x007FC0, 0x00F0C0,
        0x00C000, 0x01C000, 0x01C000, 0x018000, 0x018000, 0x018000,
        0x018000, 0x018000, 0x018000, 0x01C000, 0x01C000, 0x00C000,
        0x00F0C0, 0x007FC0, 0x001F00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_D] = {
        0x000000, 0x000000, 0x000000, 0x01F800, 0x01FE00, 0x018700,
        0x018380, 0x018180, 0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0,
        0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x018180, 0x018380,
        0x018700, 0x01FE00, 0x01F800, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_E] = {
        0x000000, 0x000000, 0x000000, 0x01FFC0, 0x01FFC0, 0x01C000,
        0x01C000, 0x01C000, 0x01C000, 0x01C000, 0x01C000, 0x01FF80,
        0x01FF80, 0x01C000, 0x01C000, 0x01C000, 0x01C000, 0x01C000,
        0x01C000, 0x01FFC0, 0x01FFC0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_I] = {
        0x000000, 0x000000, 0x000000, 0x01FF80, 0x01FF80, 0x001800,
        0x001800, 0x001800, 0x001800, 0x001800, 0x001800, 0x001800,
        0x001800, 0x001800, 0x001800, 0x001800, 0x001800, 0x001800,
        0x001800, 0x01FF80, 0x01FF80, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_K] = {
        0x000000, 0x000000, 0x000000, 0x0181C0, 0x018380, 0x018700,
        0x018E00, 0x019C00, 0x01B800, 0x01F000, 0x01E000, 0x01F000,
        0x01B800, 0x019C00, 0x018E00, 0x018700, 0x018380, 0x0181C0,
        0x0180E0, 0x018070, 0x018038, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_N] = {
        0x000000, 0x000000, 0x000000, 0x0181C0, 0x01C1C0, 0x01E1C0,
        0x01E1C0, 0x01F1C0, 0x01D9C0, 0x01D9C0, 0x01CDC0, 0x01CDC0,
        0x01C7C0, 0x01C7C0, 0x01C3C0, 0x01C3C0, 0x01C1C0, 0x01C1C0,
        0x01C1C0, 0x01C1C0, 0x01C1C0, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_O] = {
        0x000000, 0x000000, 0x000000, 0x003E00, 0x00FF00, 0x00E380,
        0x01C180, 0x0181C0, 0x0181C0, 0x0181C0, 0x0380C0, 0x0380C0,
        0x0380C0, 0x0380C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x01C180,
        0x00E380, 0x00FF00, 0x003E00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_Q] = {
        0x000000, 0x000000, 0x000000, 0x003E00, 0x00FF00, 0x00E380, 
        0x01C180, 0x0181C0, 0x0181C0, 0x0181C0, 0x0380C0, 0x0380C0, 
        0x0380C0, 0x0380C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x01C180, 
        0x00E380, 0x00FF00, 0x003E00, 0x000700, 0x000380, 0x000100, 
    },
    [OVERLAY_GLYPH_S] = {
        0x000000, 0x000000, 0x000000, 0x003E00, 0x00FF80, 0x01C180,
        0x018000, 0x018000, 0x018000, 0x01C000, 0x01F000, 0x00FE00,
        0x003F80, 0x000780, 0x0001C0, 0x0000C0, 0x0000C0, 0x0001C0,
        0x010380, 0x01FF00, 0x007E00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_T] = {
        0x000000, 0x000000, 0x000000, 0x03FFE0, 0x03FFE0, 0x001800,
        0x001800, 0x001800, 0x001800, 0x001800, 0x001800, 0x001800,
        0x001800, 0x001800, 0x001800, 0x001800, 0x001800, 0x001800,
        0x001800, 0x001800, 0x001800, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_U] = {
        0x000000, 0x000000, 0x000000, 0x0181C0, 0x0181C0, 0x0181C0,
        0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0,
        0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x0181C0, 0x018180,
        0x01C380, 0x00FF00, 0x003E00, 0x000000, 0x000000, 0x000000,
    },
    [OVERLAY_GLYPH_W] = {
        0x000000, 0x000000, 0x000000, 0x0300C0, 0x0300C0, 0x0300C0,
        0x0300C0, 0x0300C0, 0x0300C0, 0x0300C0, 0x0300C0, 0x030CC0,
        0x031EC0, 0x031EC0, 0x033FC0, 0x0333C0, 0x0333C0, 0x03E3C0,
        0x03C1C0, 0x0381C0, 0x0300C0, 0x000000, 0x000000, 0x000000,
    },
};

static void
set_glyph_color(SDL_Renderer *renderer) {
    set_overlay_label_color(renderer, SC_OVERLAY_TEXT_COLOR);
}

static void
draw_glyph(SDL_Renderer *renderer, int center_x, int center_y,
           enum overlay_glyph glyph, int scale) {
    int width = OVERLAY_GLYPH_WIDTH * scale;
    int height = OVERLAY_GLYPH_HEIGHT * scale;
    int start_x = center_x - width / 2;
    int start_y = center_y - height / 2;

    set_glyph_color(renderer);

    for (int row = 0; row < OVERLAY_GLYPH_HEIGHT; ++row) {
        uint32_t row_bits = overlay_glyph_data[glyph][row];
        for (int col = 0; col < OVERLAY_GLYPH_WIDTH; ++col) {
            if (row_bits & (1u << (OVERLAY_GLYPH_WIDTH - 1 - col))) {
                SDL_Rect pixel = {
                    .x = start_x + col * scale,
                    .y = start_y + row * scale,
                    .w = scale,
                    .h = scale,
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

static void
draw_filled_rect(SDL_Renderer *renderer, const SDL_Rect *rect, uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderFillRect(renderer, rect);
}

static void
draw_rect_outline(SDL_Renderer *renderer, const SDL_Rect *rect,
                  uint32_t color) {
    uint8_t r, g, b, a;
    color_to_rgba(color, &r, &g, &b, &a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderDrawRect(renderer, rect);
}

static int
overlay_text_width(int count, int scale) {
    if (count <= 0) {
        return 0;
    }
    return count * OVERLAY_GLYPH_WIDTH * scale
           + (count - 1) * OVERLAY_TEXT_SPACING * scale;
}

static void
draw_glyph_word(SDL_Renderer *renderer, int center_x, int center_y,
                const enum overlay_glyph *glyphs, int count, int scale) {
    if (count <= 0) {
        return;
    }

    int step = OVERLAY_GLYPH_WIDTH * scale + OVERLAY_TEXT_SPACING * scale;
    int total_width = overlay_text_width(count, scale);
    int start_x = center_x - total_width / 2 + OVERLAY_GLYPH_WIDTH * scale / 2;

    for (int i = 0; i < count; ++i) {
        draw_glyph(renderer, start_x + i * step, center_y, glyphs[i], scale);
    }
}

struct overlay_word {
    const enum overlay_glyph *glyphs;
    int count;
};

static const enum overlay_glyph overlay_label_new[] = {
    OVERLAY_GLYPH_N, OVERLAY_GLYPH_E, OVERLAY_GLYPH_W,
};

static const enum overlay_glyph overlay_label_edit[] = {
    OVERLAY_GLYPH_E, OVERLAY_GLYPH_D, OVERLAY_GLYPH_I, OVERLAY_GLYPH_T,
};

static const enum overlay_glyph overlay_label_add[] = {
    OVERLAY_GLYPH_A, OVERLAY_GLYPH_D, OVERLAY_GLYPH_D,
};

static const enum overlay_glyph overlay_label_del[] = {
    OVERLAY_GLYPH_D, OVERLAY_GLYPH_E, OVERLAY_GLYPH_L,
};

static const enum overlay_glyph overlay_label_quit[] = {
    OVERLAY_GLYPH_Q, OVERLAY_GLYPH_U, OVERLAY_GLYPH_I, OVERLAY_GLYPH_T,
};

static const enum overlay_glyph overlay_label_button[] = {
    OVERLAY_GLYPH_B, OVERLAY_GLYPH_U, OVERLAY_GLYPH_T, OVERLAY_GLYPH_T,
    OVERLAY_GLYPH_O, OVERLAY_GLYPH_N,
};

static const enum overlay_glyph overlay_label_skill[] = {
    OVERLAY_GLYPH_S, OVERLAY_GLYPH_K, OVERLAY_GLYPH_I, OVERLAY_GLYPH_L,
    OVERLAY_GLYPH_L,
};

static const enum overlay_glyph overlay_label_walk[] = {
    OVERLAY_GLYPH_W, OVERLAY_GLYPH_A, OVERLAY_GLYPH_L, OVERLAY_GLYPH_K,
};

static struct overlay_word
overlay_get_word(enum sc_touchmap_overlay_control control) {
    switch (control) {
        case SC_TOUCHMAP_OVERLAY_CONTROL_NEW:
            return (struct overlay_word) {
                overlay_label_new, ARRAY_LEN(overlay_label_new),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_EDIT:
            return (struct overlay_word) {
                overlay_label_edit, ARRAY_LEN(overlay_label_edit),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_ADD:
            return (struct overlay_word) {
                overlay_label_add, ARRAY_LEN(overlay_label_add),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_DEL:
            return (struct overlay_word) {
                overlay_label_del, ARRAY_LEN(overlay_label_del),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_QUIT:
            return (struct overlay_word) {
                overlay_label_quit, ARRAY_LEN(overlay_label_quit),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON:
            return (struct overlay_word) {
                overlay_label_button, ARRAY_LEN(overlay_label_button),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL:
            return (struct overlay_word) {
                overlay_label_skill, ARRAY_LEN(overlay_label_skill),
            };
        case SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK:
            return (struct overlay_word) {
                overlay_label_walk, ARRAY_LEN(overlay_label_walk),
            };
        default:
            return (struct overlay_word) {NULL, 0};
    }
}

static int
overlay_button_width(enum sc_touchmap_overlay_control control) {
    struct overlay_word word = overlay_get_word(control);
    return overlay_text_width(word.count, OVERLAY_GLYPH_SCALE_BUTTON)
         + 2 * OVERLAY_EDIT_PADDING;
}

SDL_Rect
sc_touchmap_overlay_get_edit_button_rect(const SDL_Rect *content_rect,
                                         bool edit_mode) {
    (void) edit_mode;
    int scale = OVERLAY_GLYPH_SCALE_BUTTON;
    int text_width = overlay_text_width(4, scale);
    int width = text_width + 2 * OVERLAY_EDIT_PADDING;
    int height = OVERLAY_GLYPH_HEIGHT * scale + 2 * OVERLAY_EDIT_PADDING;
    SDL_Rect rect = {
        .x = content_rect->x + content_rect->w - width - OVERLAY_EDIT_MARGIN,
        .y = content_rect->y + OVERLAY_EDIT_MARGIN,
        .w = width,
        .h = height,
    };
    return rect;
}

static void
draw_overlay_button(SDL_Renderer *renderer, const SDL_Rect *rect,
                    enum sc_touchmap_overlay_control control, uint32_t bg) {
    struct overlay_word word = overlay_get_word(control);
    draw_filled_rect(renderer, rect, bg);
    draw_rect_outline(renderer, rect, SC_OVERLAY_EDIT_BORDER);

    int center_x = rect->x + rect->w / 2;
    int center_y = rect->y + rect->h / 2;
    draw_glyph_word(renderer, center_x, center_y, word.glyphs, word.count,
                    OVERLAY_GLYPH_SCALE_BUTTON);
}

static void
draw_edit_button(SDL_Renderer *renderer, const SDL_Rect *rect,
                 bool has_touchmap, bool edit_mode) {
    enum sc_touchmap_overlay_control control =
        !has_touchmap ? SC_TOUCHMAP_OVERLAY_CONTROL_NEW
                      : edit_mode ? SC_TOUCHMAP_OVERLAY_CONTROL_QUIT
                                  : SC_TOUCHMAP_OVERLAY_CONTROL_EDIT;
    uint32_t bg = edit_mode ? SC_OVERLAY_EDIT_BG_ACTIVE
                            : SC_OVERLAY_EDIT_BG_COLOR;
    draw_overlay_button(renderer, rect, control, bg);
}

static SDL_Rect
overlay_get_toolbar_button_rect(const SDL_Rect *content_rect,
                                enum sc_touchmap_overlay_control control) {
    static const enum sc_touchmap_overlay_control controls[] = {
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD,
        SC_TOUCHMAP_OVERLAY_CONTROL_DEL,
        SC_TOUCHMAP_OVERLAY_CONTROL_QUIT,
    };

    int height = OVERLAY_GLYPH_HEIGHT * OVERLAY_GLYPH_SCALE_BUTTON
               + 2 * OVERLAY_EDIT_PADDING;
    int total_width = 0;
    for (size_t i = 0; i < ARRAY_LEN(controls); ++i) {
        total_width += overlay_button_width(controls[i]);
        if (i + 1 < ARRAY_LEN(controls)) {
            total_width += OVERLAY_TOOLBAR_GAP;
        }
    }

    int x = content_rect->x + content_rect->w - total_width
          - OVERLAY_EDIT_MARGIN;
    int y = content_rect->y + OVERLAY_EDIT_MARGIN;
    for (size_t i = 0; i < ARRAY_LEN(controls); ++i) {
        int width = overlay_button_width(controls[i]);
        SDL_Rect rect = {
            .x = x,
            .y = y,
            .w = width,
            .h = height,
        };
        if (controls[i] == control) {
            return rect;
        }
        x += width + OVERLAY_TOOLBAR_GAP;
    }

    return (SDL_Rect) {0, 0, 0, 0};
}

static SDL_Rect
overlay_get_add_menu_item_rect(const SDL_Rect *content_rect,
                               enum sc_touchmap_overlay_control control) {
    static const enum sc_touchmap_overlay_control controls[] = {
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON,
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL,
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK,
    };

    SDL_Rect add_rect = overlay_get_toolbar_button_rect(
        content_rect, SC_TOUCHMAP_OVERLAY_CONTROL_ADD);
    int size = add_rect.h;
    int x = add_rect.x;
    int y = add_rect.y + add_rect.h + OVERLAY_TOOLBAR_GAP;

    for (size_t i = 0; i < ARRAY_LEN(controls); ++i) {
        SDL_Rect rect = {
            .x = x,
            .y = y + (int) i * (size + OVERLAY_TOOLBAR_GAP),
            .w = overlay_button_width(controls[i]),
            .h = size,
        };
        if (controls[i] == control) {
            return rect;
        }
    }

    return (SDL_Rect) {0, 0, 0, 0};
}

static bool
point_in_rect(int32_t x, int32_t y, const SDL_Rect *rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

static void
draw_touchmap_toolbar(SDL_Renderer *renderer,
                      struct sc_touchmap_overlay *overlay,
                      const struct sc_gptm_gamepad_touchmap *touchmap,
                      const SDL_Rect *content_rect) {
    static const enum sc_touchmap_overlay_control controls[] = {
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD,
        SC_TOUCHMAP_OVERLAY_CONTROL_DEL,
        SC_TOUCHMAP_OVERLAY_CONTROL_QUIT,
    };

    for (size_t i = 0; i < ARRAY_LEN(controls); ++i) {
        SDL_Rect rect = overlay_get_toolbar_button_rect(content_rect,
                                                        controls[i]);
        uint32_t bg = controls[i] == SC_TOUCHMAP_OVERLAY_CONTROL_QUIT
                    ? SC_OVERLAY_EDIT_BG_ACTIVE : SC_OVERLAY_EDIT_BG_COLOR;
        draw_overlay_button(renderer, &rect, controls[i], bg);
    }

    if (!overlay->add_menu_open) {
        return;
    }

    static const enum sc_touchmap_overlay_control menu_items[] = {
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON,
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL,
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK,
    };

    for (size_t i = 0; i < ARRAY_LEN(menu_items); ++i) {
        SDL_Rect rect = overlay_get_add_menu_item_rect(content_rect,
                                                       menu_items[i]);
        bool disabled = menu_items[i] == SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK
                     && touchmap && touchmap->has_walk;
        uint32_t bg = disabled ? 0x40404099 : SC_OVERLAY_EDIT_BG_COLOR;
        draw_overlay_button(renderer, &rect, menu_items[i], bg);
    }
}

static void
draw_button_label(SDL_Renderer *renderer, int center_x, int center_y,
                  enum overlay_label label, int scale) {
    enum overlay_glyph glyph = OVERLAY_GLYPH_A;

    switch (label) {
        case OVERLAY_LABEL_A: glyph = OVERLAY_GLYPH_A; break;
        case OVERLAY_LABEL_B: glyph = OVERLAY_GLYPH_B; break;
        case OVERLAY_LABEL_X: glyph = OVERLAY_GLYPH_X; break;
        case OVERLAY_LABEL_Y: glyph = OVERLAY_GLYPH_Y; break;
        case OVERLAY_LABEL_LEFT: glyph = OVERLAY_GLYPH_LEFT; break;
        case OVERLAY_LABEL_RIGHT: glyph = OVERLAY_GLYPH_RIGHT; break;
        case OVERLAY_LABEL_UP: glyph = OVERLAY_GLYPH_UP; break;
        case OVERLAY_LABEL_DOWN: glyph = OVERLAY_GLYPH_DOWN; break;
        case OVERLAY_LABEL_BACK: glyph = OVERLAY_GLYPH_BACK; break;
        case OVERLAY_LABEL_START: glyph = OVERLAY_GLYPH_START; break;
        case OVERLAY_LABEL_GUIDE: glyph = OVERLAY_GLYPH_GUIDE; break;
        case OVERLAY_LABEL_L1:
        case OVERLAY_LABEL_L2:
        case OVERLAY_LABEL_L3:
        case OVERLAY_LABEL_R1:
        case OVERLAY_LABEL_R2:
        case OVERLAY_LABEL_R3:
            break;
        default:
            return;
    }

    if (label == OVERLAY_LABEL_L1 || label == OVERLAY_LABEL_L2
            || label == OVERLAY_LABEL_L3 || label == OVERLAY_LABEL_R1
            || label == OVERLAY_LABEL_R2 || label == OVERLAY_LABEL_R3) {
        enum overlay_glyph first = (label == OVERLAY_LABEL_L1
                || label == OVERLAY_LABEL_L2 || label == OVERLAY_LABEL_L3)
                                  ? OVERLAY_GLYPH_L
                                  : OVERLAY_GLYPH_R;
        enum overlay_glyph second = OVERLAY_GLYPH_1;
        switch (label) {
            case OVERLAY_LABEL_L2:
            case OVERLAY_LABEL_R2:
                second = OVERLAY_GLYPH_2;
                break;
            case OVERLAY_LABEL_L3:
            case OVERLAY_LABEL_R3:
                second = OVERLAY_GLYPH_3;
                break;
            default:
                second = OVERLAY_GLYPH_1;
                break;
        }

        int offset = (OVERLAY_GLYPH_WIDTH / 2 + OVERLAY_GLYPH_SPACING) * scale;
        draw_glyph(renderer, center_x - offset, center_y, first, scale);
        draw_glyph(renderer, center_x + offset, center_y, second, scale);
        return;
    }

    draw_glyph(renderer, center_x, center_y, glyph, scale);
}


bool
sc_touchmap_overlay_init(struct sc_touchmap_overlay *overlay,
                         SDL_Renderer *renderer) {
    (void)renderer; // Not used in this simple implementation
    overlay->overlay_texture = NULL;
    overlay->last_size.width = 0;
    overlay->last_size.height = 0;
    overlay->enabled = false;
    overlay->edit_mode = false;
    overlay->add_menu_open = false;
    return true;
}

void
sc_touchmap_overlay_destroy(struct sc_touchmap_overlay *overlay) {
    if (overlay->overlay_texture) {
        SDL_DestroyTexture(overlay->overlay_texture);
        overlay->overlay_texture = NULL;
    }
}

#ifndef SC_TEST
static
#endif
struct sc_point
sc_touchmap_overlay_transform_point(const struct sc_point *point,
                                    const struct sc_size *frame_size,
                                    const SDL_Rect *content_rect,
                                    enum sc_orientation orientation) {
    struct sc_point oriented = *point;
    int32_t fw = frame_size->width;
    int32_t fh = frame_size->height;

    switch (orientation) {
        case SC_ORIENTATION_0:
            break;
        case SC_ORIENTATION_90:
            oriented.x = fh - point->y;
            oriented.y = point->x;
            break;
        case SC_ORIENTATION_180:
            oriented.x = fw - point->x;
            oriented.y = fh - point->y;
            break;
        case SC_ORIENTATION_270:
            oriented.x = point->y;
            oriented.y = fw - point->x;
            break;
        case SC_ORIENTATION_FLIP_0:
            oriented.x = fw - point->x;
            oriented.y = point->y;
            break;
        case SC_ORIENTATION_FLIP_90:
            oriented.x = fh - point->y;
            oriented.y = fw - point->x;
            break;
        case SC_ORIENTATION_FLIP_180:
            oriented.x = point->x;
            oriented.y = fh - point->y;
            break;
        default:
            oriented.x = point->y;
            oriented.y = point->x;
            break;
    }

    if (sc_orientation_is_swap(orientation)) {
        int32_t tmp = fw;
        fw = fh;
        fh = tmp;
    }

    struct sc_point result = {
        .x = content_rect->x + (int64_t) oriented.x * content_rect->w / fw,
        .y = content_rect->y + (int64_t) oriented.y * content_rect->h / fh,
    };

    return result;
}

#ifndef SC_TEST
static
#endif
int32_t
sc_touchmap_overlay_transform_radius(int32_t radius,
                                     const struct sc_size *frame_size,
                                     const SDL_Rect *content_rect,
                                     enum sc_orientation orientation) {
    int32_t fw = frame_size->width;
    int32_t fh = frame_size->height;

    if (sc_orientation_is_swap(orientation)) {
        int32_t tmp = fw;
        fw = fh;
        fh = tmp;
    }

    int32_t sx = (int64_t) radius * content_rect->w / fw;
    int32_t sy = (int64_t) radius * content_rect->h / fh;
    return sx < sy ? sx : sy;
}

static void
draw_selection_circle(SDL_Renderer *renderer, int32_t x, int32_t y,
                      int32_t radius) {
    for (int32_t offset = 0; offset < 3; ++offset) {
        draw_circle_outline(renderer, x, y, radius + offset,
                            SC_OVERLAY_SELECTION_COLOR);
    }
}

static void
draw_touchmap_selection(SDL_Renderer *renderer,
                        const struct sc_gptm_gamepad_touchmap *touchmap,
                        const struct sc_size *frame_size,
                        const SDL_Rect *content_rect,
                        enum sc_orientation orientation,
                        const struct sc_touchmap_editor *touchmap_editor) {
    if (!touchmap_editor) {
        return;
    }

    struct sc_touchmap_editor_selection selection =
        touchmap_editor->selection;
    switch (selection.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER: {
            if (!touchmap->has_walk) {
                break;
            }
            struct sc_point center = sc_touchmap_overlay_transform_point(
                &touchmap->walk.center, frame_size, content_rect, orientation);
            int32_t radius = sc_touchmap_overlay_transform_radius(
                SC_TOUCHMAP_MIN_RADIUS + 4, frame_size, content_rect,
                orientation);
            draw_selection_circle(renderer, center.x, center.y, radius);
            break;
        }
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS: {
            if (!touchmap->has_walk) {
                break;
            }
            struct sc_point center = sc_touchmap_overlay_transform_point(
                &touchmap->walk.center, frame_size, content_rect, orientation);
            int32_t radius = sc_touchmap_overlay_transform_radius(
                touchmap->walk.radius + 4, frame_size, content_rect,
                orientation);
            draw_selection_circle(renderer, center.x, center.y, radius);
            break;
        }
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER:
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS: {
            int index = selection.button_index;
            if (index < 0 || index >= touchmap->button_cnt) {
                break;
            }

            const struct sc_gptm_touch_button *btn = &touchmap->buttons[index];
            struct sc_point center = sc_touchmap_overlay_transform_point(
                &btn->center, frame_size, content_rect, orientation);
            int32_t base_radius =
                selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS
                    && btn->radius > 0 ? btn->radius : SC_TOUCHMAP_MIN_RADIUS;
            int32_t radius = sc_touchmap_overlay_transform_radius(
                base_radius + 4, frame_size, content_rect, orientation);
            draw_selection_circle(renderer, center.x, center.y, radius);
            break;
        }
        default:
            break;
    }
}

bool
sc_touchmap_overlay_render(struct sc_touchmap_overlay *overlay,
                           SDL_Renderer *renderer,
                           const struct sc_gptm_gamepad_touchmap *touchmap,
                           const struct sc_size *frame_size,
                           const SDL_Rect *content_rect,
                           enum sc_orientation orientation,
                           const struct sc_touchmap_editor *touchmap_editor) {
    if (!overlay->enabled || !content_rect || !frame_size
            || !frame_size->width || !frame_size->height) {
        return true;
    }

    if (!touchmap) {
        SDL_Rect edit_rect = sc_touchmap_overlay_get_edit_button_rect(
            content_rect, false);
        draw_edit_button(renderer, &edit_rect, false, false);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        return true;
    }

    // Draw walk control (outer circle)
    if (touchmap->has_walk && touchmap->walk.radius > 0) {
        struct sc_point walk_center = sc_touchmap_overlay_transform_point(
            &touchmap->walk.center, frame_size, content_rect, orientation);
        int32_t walk_radius = sc_touchmap_overlay_transform_radius(
            touchmap->walk.radius, frame_size, content_rect, orientation);

        draw_filled_circle(renderer,
                          walk_center.x,
                          walk_center.y,
                          walk_radius,
                          SC_OVERLAY_WALK_COLOR);

        // Draw walk control outline
        draw_circle_outline(renderer,
                           walk_center.x,
                           walk_center.y,
                           walk_radius,
                           0xFFFFFFB0); // White outline
    }

    // Draw current position for walk control
    if (touchmap->has_walk && touchmap->walk.touch_down) {
        struct sc_point walk_pos = sc_touchmap_overlay_transform_point(
            &touchmap->walk.current_pos, frame_size, content_rect, orientation);
        int32_t walk_pos_radius = sc_touchmap_overlay_transform_radius(
            OVERLAY_WALK_POS_RADIUS, frame_size, content_rect, orientation);
        if (walk_pos_radius < 1) {
            walk_pos_radius = 1;
        }
        draw_filled_circle(renderer,
                           walk_pos.x,
                           walk_pos.y,
                           walk_pos_radius,
                           0xFFFFFFE0); // White for current pos

    }

    if (touchmap->has_walk && touchmap->walk.radius > 0) {
        struct sc_point walk_center = sc_touchmap_overlay_transform_point(
            &touchmap->walk.center, frame_size, content_rect, orientation);
        int walk_label_scale = OVERLAY_GLYPH_SCALE_WALK;
        int offset = (OVERLAY_GLYPH_HEIGHT + OVERLAY_GLYPH_SPACING) / 2
                     * walk_label_scale;

        draw_button_label(renderer, walk_center.x - offset, walk_center.y,
                          OVERLAY_LABEL_LEFT, walk_label_scale);
        draw_button_label(renderer, walk_center.x + offset, walk_center.y,
                          OVERLAY_LABEL_RIGHT, walk_label_scale);
        draw_button_label(renderer, walk_center.x, walk_center.y - offset,
                          OVERLAY_LABEL_UP, walk_label_scale);
        draw_button_label(renderer, walk_center.x, walk_center.y + offset,
                          OVERLAY_LABEL_DOWN, walk_label_scale);
    }

    // Draw button mappings
    for (int i = 0; i < touchmap->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &touchmap->buttons[i];
        bool bound = sc_gptm_touch_button_is_bound(btn);
        uint32_t color = !bound ? SC_OVERLAY_UNBOUND_COLOR
                       : btn->is_skill ? SC_OVERLAY_SKILL_COLOR
                                       : SC_OVERLAY_BUTTON_COLOR;

        struct sc_point btn_center = sc_touchmap_overlay_transform_point(
            &btn->center, frame_size, content_rect, orientation);

        // Draw button area as filled circle with transparency
        int button_radius = OVERLAY_BUTTON_RADIUS;
        draw_filled_circle(renderer, btn_center.x, btn_center.y,
                          button_radius,
                          color);

        // Draw outline circle (solid)
        uint32_t outline_color = !bound ? 0xFF3434FF
                               : btn->is_skill ? 0xFFFFFFC0 : 0xFFFFFFA0;
        draw_circle_outline(renderer, btn_center.x, btn_center.y,
                           button_radius,
                           outline_color);

        if (btn->is_skill && btn->radius > 0 && overlay->edit_mode) {
            int32_t skill_radius = sc_touchmap_overlay_transform_radius(
                btn->radius, frame_size, content_rect, orientation);
            draw_dashed_circle_outline(renderer, btn_center.x, btn_center.y,
                                       skill_radius, SC_OVERLAY_DASH_COLOR);
        }

        // Draw button indicator if touched
        if (btn->touch_down) {
            struct sc_point btn_pos = sc_touchmap_overlay_transform_point(
                &btn->current_pos, frame_size, content_rect, orientation);
            draw_filled_circle(renderer, btn_pos.x, btn_pos.y,
                              OVERLAY_PRESSED_RADIUS, outline_color);
        }

        enum overlay_label label = button_value_to_label(btn->button);
        draw_button_label(renderer, btn_center.x, btn_center.y,
                          label, OVERLAY_GLYPH_SCALE_BUTTON);
    }

    if (overlay->edit_mode) {
        draw_touchmap_selection(renderer, touchmap, frame_size, content_rect,
                                orientation, touchmap_editor);
        draw_touchmap_toolbar(renderer, overlay, touchmap, content_rect);
    } else {
        SDL_Rect edit_rect = sc_touchmap_overlay_get_edit_button_rect(
            content_rect, false);
        draw_edit_button(renderer, &edit_rect, true, false);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    return true;
}

void
sc_touchmap_overlay_toggle(struct sc_touchmap_overlay *overlay) {
    overlay->enabled = !overlay->enabled;
    if (!overlay->enabled) {
        overlay->edit_mode = false;
        overlay->add_menu_open = false;
    }
    LOGI("Touchmap overlay %s", overlay->enabled ? "enabled" : "disabled");
}

void
sc_touchmap_overlay_set_enabled(struct sc_touchmap_overlay *overlay,
                                bool enabled) {
    overlay->enabled = enabled;
    if (!enabled) {
        overlay->edit_mode = false;
        overlay->add_menu_open = false;
    }
}

bool
sc_touchmap_overlay_is_enabled(const struct sc_touchmap_overlay *overlay) {
    return overlay->enabled;
}

void
sc_touchmap_overlay_set_edit_mode(struct sc_touchmap_overlay *overlay,
                                  bool edit_mode) {
    overlay->edit_mode = edit_mode;
    if (!edit_mode) {
        overlay->add_menu_open = false;
    }
}

bool
sc_touchmap_overlay_is_edit_mode(const struct sc_touchmap_overlay *overlay) {
    return overlay->edit_mode;
}

enum sc_touchmap_overlay_control
sc_touchmap_overlay_hit_control(struct sc_touchmap_overlay *overlay,
                                const struct sc_gptm_gamepad_touchmap *touchmap,
                                const SDL_Rect *content_rect,
                                int32_t x, int32_t y) {
    if (!overlay->enabled || !content_rect) {
        return SC_TOUCHMAP_OVERLAY_CONTROL_NONE;
    }

    if (!touchmap) {
        SDL_Rect rect = sc_touchmap_overlay_get_edit_button_rect(content_rect,
                                                                 false);
        return point_in_rect(x, y, &rect) ? SC_TOUCHMAP_OVERLAY_CONTROL_NEW
                                          : SC_TOUCHMAP_OVERLAY_CONTROL_NONE;
    }

    if (!overlay->edit_mode) {
        SDL_Rect rect = sc_touchmap_overlay_get_edit_button_rect(content_rect,
                                                                 false);
        return point_in_rect(x, y, &rect) ? SC_TOUCHMAP_OVERLAY_CONTROL_EDIT
                                          : SC_TOUCHMAP_OVERLAY_CONTROL_NONE;
    }

    if (overlay->add_menu_open) {
        static const enum sc_touchmap_overlay_control menu_items[] = {
            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON,
            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL,
            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK,
        };

        for (size_t i = 0; i < ARRAY_LEN(menu_items); ++i) {
            SDL_Rect rect = overlay_get_add_menu_item_rect(content_rect,
                                                           menu_items[i]);
            if (point_in_rect(x, y, &rect)) {
                overlay->add_menu_open = false;
                return menu_items[i];
            }
        }
    }

    static const enum sc_touchmap_overlay_control toolbar_items[] = {
        SC_TOUCHMAP_OVERLAY_CONTROL_ADD,
        SC_TOUCHMAP_OVERLAY_CONTROL_DEL,
        SC_TOUCHMAP_OVERLAY_CONTROL_QUIT,
    };

    for (size_t i = 0; i < ARRAY_LEN(toolbar_items); ++i) {
        SDL_Rect rect = overlay_get_toolbar_button_rect(content_rect,
                                                        toolbar_items[i]);
        if (point_in_rect(x, y, &rect)) {
            if (toolbar_items[i] == SC_TOUCHMAP_OVERLAY_CONTROL_ADD) {
                overlay->add_menu_open = !overlay->add_menu_open;
            } else {
                overlay->add_menu_open = false;
            }
            return toolbar_items[i];
        }
    }

    overlay->add_menu_open = false;
    return SC_TOUCHMAP_OVERLAY_CONTROL_NONE;
}
