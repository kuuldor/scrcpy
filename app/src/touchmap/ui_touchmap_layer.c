#include "ui_touchmap_layer.h"

#include <math.h>

#include <SDL2/SDL.h>

#include "input_manager.h"
#include "ui/ui_draw.h"
#include "ui/ui_context.h"
#include "ui/ui_geom.h"
#include "ui/ui_id.h"
#include "touchmap/touchmap_state.h"

#define SC_UI_TOUCHMAP_CIRCLE_POINTS 32
#define SC_UI_TOUCHMAP_EDIT_MARGIN 8
#define SC_UI_TOUCHMAP_TOOLBAR_PADDING 6
#define SC_UI_TOUCHMAP_TOOLBAR_GAP 6
#define SC_UI_TOUCHMAP_MENU_GAP 6

#define SC_UI_TOUCHMAP_WALK_COLOR       0xFFFFFF40
#define SC_UI_TOUCHMAP_BUTTON_COLOR     0x6BFF6B55
#define SC_UI_TOUCHMAP_SKILL_COLOR      0x6B6BFF70
#define SC_UI_TOUCHMAP_UNBOUND_COLOR    0xFF3434A0
#define SC_UI_TOUCHMAP_TEXT_COLOR       0xFFFFFFB0
#define SC_UI_TOUCHMAP_DASH_COLOR       0xFFFFFFD0
#define SC_UI_TOUCHMAP_SELECTION_COLOR  0xFFD23FFF

#define SC_UI_TOUCHMAP_PRESSED_RADIUS 10
#define SC_UI_TOUCHMAP_WALK_POS_RADIUS 5
#define SC_UI_TOUCHMAP_GLYPH_SCALE_BUTTON 1
#define SC_UI_TOUCHMAP_GLYPH_SCALE_WALK 3
#define SC_UI_TOUCHMAP_GLYPH_SPACING (-4)
#define SC_UI_TOUCHMAP_GLYPH_WIDTH 24
#define SC_UI_TOUCHMAP_GLYPH_HEIGHT 24
#define SC_UI_TOUCHMAP_ICON_MAX_WIDTH (SC_UI_TOUCHMAP_GLYPH_WIDTH * 2)
#define SC_UI_TOUCHMAP_WALK_ICON_WIDTH (SC_UI_TOUCHMAP_GLYPH_WIDTH * 3)
#define SC_UI_TOUCHMAP_WALK_ICON_HEIGHT (SC_UI_TOUCHMAP_GLYPH_HEIGHT * 3)

enum sc_ui_touchmap_label {
    SC_UI_TOUCHMAP_LABEL_NONE,
    SC_UI_TOUCHMAP_LABEL_A,
    SC_UI_TOUCHMAP_LABEL_B,
    SC_UI_TOUCHMAP_LABEL_X,
    SC_UI_TOUCHMAP_LABEL_Y,
    SC_UI_TOUCHMAP_LABEL_L1,
    SC_UI_TOUCHMAP_LABEL_R1,
    SC_UI_TOUCHMAP_LABEL_L2,
    SC_UI_TOUCHMAP_LABEL_R2,
    SC_UI_TOUCHMAP_LABEL_L3,
    SC_UI_TOUCHMAP_LABEL_R3,
    SC_UI_TOUCHMAP_LABEL_LEFT,
    SC_UI_TOUCHMAP_LABEL_RIGHT,
    SC_UI_TOUCHMAP_LABEL_UP,
    SC_UI_TOUCHMAP_LABEL_DOWN,
    SC_UI_TOUCHMAP_LABEL_BACK,
    SC_UI_TOUCHMAP_LABEL_START,
    SC_UI_TOUCHMAP_LABEL_GUIDE,
    SC_UI_TOUCHMAP_LABEL_TOUCHPAD,
};

enum sc_ui_touchmap_glyph {
    SC_UI_TOUCHMAP_GLYPH_A,
    SC_UI_TOUCHMAP_GLYPH_B,
    SC_UI_TOUCHMAP_GLYPH_X,
    SC_UI_TOUCHMAP_GLYPH_Y,
    SC_UI_TOUCHMAP_GLYPH_L,
    SC_UI_TOUCHMAP_GLYPH_R,
    SC_UI_TOUCHMAP_GLYPH_1,
    SC_UI_TOUCHMAP_GLYPH_2,
    SC_UI_TOUCHMAP_GLYPH_3,
    SC_UI_TOUCHMAP_GLYPH_LEFT,
    SC_UI_TOUCHMAP_GLYPH_RIGHT,
    SC_UI_TOUCHMAP_GLYPH_UP,
    SC_UI_TOUCHMAP_GLYPH_DOWN,
    SC_UI_TOUCHMAP_GLYPH_BACK,
    SC_UI_TOUCHMAP_GLYPH_START,
    SC_UI_TOUCHMAP_GLYPH_GUIDE,
    SC_UI_TOUCHMAP_GLYPH_TOUCHPAD,
};

static const uint64_t sc_ui_touchmap_glyph_data[][SC_UI_TOUCHMAP_GLYPH_HEIGHT] = {
    [SC_UI_TOUCHMAP_GLYPH_A] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x007800, 0x007800,
        0x006C00, 0x00EC00, 0x00CC00, 0x00CE00, 0x00C600, 0x01C600,
        0x018600, 0x018700, 0x038300, 0x03FF00, 0x03FF80, 0x070380,
        0x070180, 0x0601C0, 0x0601C0, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_B] = {
        0x000000, 0x000000, 0x000000, 0x01FE00, 0x01FF00, 0x018380,
        0x0181C0, 0x0181C0, 0x0181C0, 0x018180, 0x018380, 0x01FF00,
        0x01FF00, 0x018180, 0x0180C0, 0x0180C0, 0x0180C0, 0x0180C0,
        0x0181C0, 0x01FF80, 0x01FE00, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_X] = {
        0x000000, 0x000000, 0x000000, 0x0701C0, 0x030380, 0x018300,
        0x01C700, 0x00C600, 0x00EC00, 0x007C00, 0x007800, 0x003800,
        0x007800, 0x007C00, 0x00EC00, 0x00C600, 0x018700, 0x038300,
        0x030380, 0x070180, 0x0601C0, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_Y] = {
        0x000000, 0x000000, 0x000000, 0x0601C0, 0x070180, 0x030380,
        0x038300, 0x018600, 0x00C600, 0x00CC00, 0x007C00, 0x007800,
        0x003800, 0x003000, 0x003000, 0x003000, 0x003000, 0x003000,
        0x003000, 0x003000, 0x003000, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_L] = {
        0x000000, 0x000000, 0x000000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000, 0x00C000,
        0x00C000, 0x00FFC0, 0x00FFC0, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_R] = {
        0x000000, 0x000000, 0x000000, 0x03FC00, 0x03FE00, 0x030700,
        0x030300, 0x030300, 0x030300, 0x030300, 0x030700, 0x03FE00,
        0x03FC00, 0x030E00, 0x030600, 0x030700, 0x030300, 0x030380,
        0x030180, 0x0301C0, 0x0300C0, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_1] = {
        0x000000, 0x000000, 0x000000, 0x003C00, 0x00FC00, 0x00CC00,
        0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00,
        0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00, 0x000C00,
        0x000C00, 0x00FFC0, 0x00FFC0, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_2] = {
        0x000000, 0x000000, 0x000000, 0x007C00, 0x01FF00, 0x018780,
        0x000380, 0x000180, 0x000180, 0x000180, 0x000380, 0x000300,
        0x000600, 0x000C00, 0x001C00, 0x003800, 0x007000, 0x00E000,
        0x01C000, 0x01FF80, 0x01FF80, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_3] = {
        0x000000, 0x000000, 0x000000, 0x007C00, 0x01FF00, 0x018380,
        0x000380, 0x000180, 0x000180, 0x000380, 0x000700, 0x003E00,
        0x003E00, 0x000380, 0x000180, 0x0001C0, 0x0001C0, 0x000180,
        0x010380, 0x01FF00, 0x00FE00, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_LEFT] = {
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x030000,
        0x070000, 0x0E0000, 0x1E0000, 0x3FFFFC, 0x3FFFFC, 0x1FFFF8,
        0x0E0000, 0x070000, 0x038000, 0x010000, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_RIGHT] = {
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x0001C0,
        0x0001E0, 0x0000F0, 0x000078, 0x3FFFF8, 0x3FFFFC, 0x1FFFF8,
        0x000070, 0x0000E0, 0x0001C0, 0x000080, 0x000000, 0x000000,
        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_UP] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x007C00, 0x00FE00,
        0x01FF00, 0x03BB80, 0x013980, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_DOWN] = {
        0x000000, 0x000000, 0x000000, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x003800, 0x003800,
        0x003800, 0x003800, 0x003800, 0x003800, 0x03B980, 0x01FB80,
        0x00FF00, 0x007E00, 0x003C00, 0x001800, 0x000000, 0x000000,
    },
    [SC_UI_TOUCHMAP_GLYPH_BACK] = {
        0x007E00, 0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC,
        0x7800FE, 0x7800FE, 0x79FFFE, 0xF9FFFF, 0xF9001F, 0xF9001F,
        0xF91F9F, 0xF91F9F, 0xFF1F9F, 0x7F1F9E, 0x7F001E, 0x7F001E,
        0x3FFFFC, 0x1FFFF8, 0x0FFFF0, 0x07FFE0, 0x03FFC0, 0x007E00,
    },
    [SC_UI_TOUCHMAP_GLYPH_START] = {
        0x007E00, 0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC,
        0x78001E, 0x78001E, 0x7FFFFE, 0xFFFFFF, 0xFFFFFF, 0xF8001F,
        0xF8001F, 0xFFFFFF, 0xFFFFFF, 0x7FFFFE, 0x78001E, 0x78001E,
        0x3FFFFC, 0x1FFFF8, 0x0FFFF0, 0x07FFE0, 0x03FFC0, 0x007E00,
    },
    [SC_UI_TOUCHMAP_GLYPH_GUIDE] = {
        0x001800, 0x001800, 0x003C00, 0x007E00, 0x00FF00, 0x01FF80,
        0x03FFC0, 0x07FFE0, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC, 0x7FFFFE,
        0xFFFFFF, 0x0FFFF0, 0x0FFFF0, 0x0FFFF0, 0x0FC3F0, 0x0FC3F0,
        0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0, 0x0FC3F0,
    },
    [SC_UI_TOUCHMAP_GLYPH_TOUCHPAD] = {
        0x000000, 0x000000, 0x000000, 0x0FFFF0, 0x1FFFF8, 0x3FFFFC,
        0x70000E, 0x600006, 0x618186, 0x600006, 0x660666, 0x600006,
        0x618186, 0x600006, 0x660666, 0x600006, 0x618186, 0x600006,
        0x70000E, 0x3FFFFC, 0x1FFFF8, 0x0FFFF0, 0x000000, 0x000000,
    },
};

static struct sc_ui_color
sc_ui_touchmap_color_from_u32(uint32_t color) {
    return sc_ui_color_rgba((color >> 24) & 0xFF, (color >> 16) & 0xFF,
                            (color >> 8) & 0xFF, color & 0xFF);
}

static bool
sc_ui_touchmap_set_color(SDL_Renderer *renderer, uint32_t color) {
    struct sc_ui_color rgba = sc_ui_touchmap_color_from_u32(color);
    return SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a)
        == 0;
}

static bool
sc_ui_touchmap_logical_to_drawable_point(const struct sc_ui_render_ctx *ctx,
                                         struct sc_point logical,
                                         struct sc_point *out) {
    if (!ctx->ui) {
        *out = logical;
        return true;
    }

    return sc_ui_context_logical_to_drawable_point(ctx->ui, logical, out);
}

static int32_t
sc_ui_touchmap_logical_to_drawable_length(const struct sc_ui_render_ctx *ctx,
                                          int32_t value) {
    if (!ctx->ui) {
        return value;
    }

    return sc_ui_context_logical_to_drawable_length(ctx->ui, value);
}

static bool
sc_ui_touchmap_draw_filled_circle(const struct sc_ui_render_ctx *ctx,
                                  struct sc_point center, int32_t radius,
                                  uint32_t color) {
    SDL_Renderer *renderer = ctx->renderer;
    struct sc_point drawable_center;
    if (!sc_ui_touchmap_logical_to_drawable_point(ctx, center,
                                                  &drawable_center)) {
        return true;
    }

    int32_t drawable_radius =
        sc_ui_touchmap_logical_to_drawable_length(ctx, radius);
    if (drawable_radius < 1) {
        drawable_radius = 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_touchmap_set_color(renderer, color)) {
        return false;
    }

    for (int32_t y = -drawable_radius; y <= drawable_radius; ++y) {
        int32_t x = (int32_t) sqrt((double) drawable_radius * drawable_radius
                                   - (double) y * y);
        if (SDL_RenderDrawLine(renderer, drawable_center.x - x,
                               drawable_center.y + y,
                               drawable_center.x + x,
                               drawable_center.y + y)) {
            return false;
        }
    }

    return true;
}

static bool
sc_ui_touchmap_draw_circle_outline(const struct sc_ui_render_ctx *ctx,
                                   struct sc_point center, int32_t radius,
                                   uint32_t color, bool dashed) {
    SDL_Renderer *renderer = ctx->renderer;
    struct sc_point drawable_center;
    if (!sc_ui_touchmap_logical_to_drawable_point(ctx, center,
                                                  &drawable_center)) {
        return true;
    }

    int32_t drawable_radius =
        sc_ui_touchmap_logical_to_drawable_length(ctx, radius);
    if (drawable_radius < 1) {
        drawable_radius = 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (!sc_ui_touchmap_set_color(renderer, color)) {
        return false;
    }

    double angle_step = 2.0 * M_PI / SC_UI_TOUCHMAP_CIRCLE_POINTS;
    for (int i = 0; i < SC_UI_TOUCHMAP_CIRCLE_POINTS; ++i) {
        if (dashed && i % 2) {
            continue;
        }

        double angle1 = i * angle_step;
        double angle2 = (i + 1) * angle_step;
        int x1 = drawable_center.x + (int) (drawable_radius * cos(angle1));
        int y1 = drawable_center.y + (int) (drawable_radius * sin(angle1));
        int x2 = drawable_center.x + (int) (drawable_radius * cos(angle2));
        int y2 = drawable_center.y + (int) (drawable_radius * sin(angle2));
        if (SDL_RenderDrawLine(renderer, x1, y1, x2, y2)) {
            return false;
        }
    }

    return true;
}

static bool
sc_ui_touchmap_draw_selection_circle(const struct sc_ui_render_ctx *ctx,
                                     struct sc_point center, int32_t radius) {
    for (int32_t offset = 0; offset < 3; ++offset) {
        if (!sc_ui_touchmap_draw_circle_outline(ctx, center, radius + offset,
                                                SC_UI_TOUCHMAP_SELECTION_COLOR,
                                                false)) {
            return false;
        }
    }

    return true;
}

static enum sc_ui_touchmap_label
sc_ui_touchmap_layer_get_button_label(uint8_t button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return SC_UI_TOUCHMAP_LABEL_A;
        case SDL_CONTROLLER_BUTTON_B: return SC_UI_TOUCHMAP_LABEL_B;
        case SDL_CONTROLLER_BUTTON_X: return SC_UI_TOUCHMAP_LABEL_X;
        case SDL_CONTROLLER_BUTTON_Y: return SC_UI_TOUCHMAP_LABEL_Y;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return SC_UI_TOUCHMAP_LABEL_L3;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return SC_UI_TOUCHMAP_LABEL_R3;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return SC_UI_TOUCHMAP_LABEL_L1;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return SC_UI_TOUCHMAP_LABEL_R1;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return SC_UI_TOUCHMAP_LABEL_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return SC_UI_TOUCHMAP_LABEL_RIGHT;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return SC_UI_TOUCHMAP_LABEL_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return SC_UI_TOUCHMAP_LABEL_DOWN;
        case SDL_CONTROLLER_BUTTON_BACK: return SC_UI_TOUCHMAP_LABEL_BACK;
        case SDL_CONTROLLER_BUTTON_START: return SC_UI_TOUCHMAP_LABEL_START;
        case SDL_CONTROLLER_BUTTON_GUIDE: return SC_UI_TOUCHMAP_LABEL_GUIDE;
        case SDL_CONTROLLER_BUTTON_TOUCHPAD: return SC_UI_TOUCHMAP_LABEL_TOUCHPAD;
        default:
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                return SC_UI_TOUCHMAP_LABEL_L2;
            }
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                return SC_UI_TOUCHMAP_LABEL_R2;
            }
            return SC_UI_TOUCHMAP_LABEL_NONE;
    }
}

static void
sc_ui_touchmap_compose_glyph(uint64_t *rows, int icon_width,
                             const uint64_t *glyph_rows, int glyph_x) {
    int shift = icon_width - glyph_x - SC_UI_TOUCHMAP_GLYPH_WIDTH;
    for (int row = 0; row < SC_UI_TOUCHMAP_GLYPH_HEIGHT; ++row) {
        rows[row] |= glyph_rows[row] << shift;
    }
}

static void
sc_ui_touchmap_compose_glyph_at(uint64_t *rows, int icon_width, int icon_height,
                                const uint64_t *glyph_rows, int glyph_x,
                                int glyph_y) {
    if (glyph_y < 0 || glyph_y + SC_UI_TOUCHMAP_GLYPH_HEIGHT > icon_height) {
        return;
    }

    int shift = icon_width - glyph_x - SC_UI_TOUCHMAP_GLYPH_WIDTH;
    for (int row = 0; row < SC_UI_TOUCHMAP_GLYPH_HEIGHT; ++row) {
        rows[glyph_y + row] |= glyph_rows[row] << shift;
    }
}

static bool
sc_ui_touchmap_compose_label_icon(enum sc_ui_touchmap_label label,
                                  uint64_t *rows, int *out_width) {
    SDL_memset(rows, 0, sizeof(uint64_t) * SC_UI_TOUCHMAP_GLYPH_HEIGHT);

    switch (label) {
        case SC_UI_TOUCHMAP_LABEL_A:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_A],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_B:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_B],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_X:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_X],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_Y:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_Y],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_LEFT:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_LEFT],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_RIGHT:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_RIGHT],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_UP:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_UP],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_DOWN:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_DOWN],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_BACK:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_BACK],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_START:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_START],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_GUIDE:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_GUIDE],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_TOUCHPAD:
            *out_width = SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[
                                             SC_UI_TOUCHMAP_GLYPH_TOUCHPAD],
                                         0);
            return true;
        case SC_UI_TOUCHMAP_LABEL_L1:
        case SC_UI_TOUCHMAP_LABEL_L2:
        case SC_UI_TOUCHMAP_LABEL_L3:
        case SC_UI_TOUCHMAP_LABEL_R1:
        case SC_UI_TOUCHMAP_LABEL_R2:
        case SC_UI_TOUCHMAP_LABEL_R3: {
            enum sc_ui_touchmap_glyph first =
                label == SC_UI_TOUCHMAP_LABEL_L1
                    || label == SC_UI_TOUCHMAP_LABEL_L2
                    || label == SC_UI_TOUCHMAP_LABEL_L3
                ? SC_UI_TOUCHMAP_GLYPH_L
                : SC_UI_TOUCHMAP_GLYPH_R;
            enum sc_ui_touchmap_glyph second = SC_UI_TOUCHMAP_GLYPH_1;
            switch (label) {
                case SC_UI_TOUCHMAP_LABEL_L2:
                case SC_UI_TOUCHMAP_LABEL_R2:
                    second = SC_UI_TOUCHMAP_GLYPH_2;
                    break;
                case SC_UI_TOUCHMAP_LABEL_L3:
                case SC_UI_TOUCHMAP_LABEL_R3:
                    second = SC_UI_TOUCHMAP_GLYPH_3;
                    break;
                default:
                    break;
            }

            int first_x = 0;
            int second_x = SC_UI_TOUCHMAP_GLYPH_WIDTH
                         + 2 * SC_UI_TOUCHMAP_GLYPH_SPACING;
            *out_width = second_x + SC_UI_TOUCHMAP_GLYPH_WIDTH;
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[first],
                                         first_x);
            sc_ui_touchmap_compose_glyph(rows, *out_width,
                                         sc_ui_touchmap_glyph_data[second],
                                         second_x);
            return true;
        }
        default:
            *out_width = 0;
            return false;
    }
}

static bool
sc_ui_touchmap_draw_button_label(const struct sc_ui_render_ctx *ctx,
                                 struct sc_point center,
                                 enum sc_ui_touchmap_label label,
                                 int scale) {
    uint64_t rows[SC_UI_TOUCHMAP_GLYPH_HEIGHT];
    int width;
    if (!sc_ui_touchmap_compose_label_icon(label, rows, &width)) {
        return true;
    }

    SDL_Rect rect = {
        .w = width * scale,
        .h = SC_UI_TOUCHMAP_GLYPH_HEIGHT * scale,
    };
    rect.x = center.x - rect.w / 2;
    rect.y = center.y - rect.h / 2;
    return sc_ui_draw_bitmap_icon_in_rect(ctx, &rect, rows, width,
                                          SC_UI_TOUCHMAP_GLYPH_HEIGHT,
                                          sc_ui_touchmap_color_from_u32(
                                              SC_UI_TOUCHMAP_TEXT_COLOR));
}

static bool
sc_ui_touchmap_draw_walk_icon(const struct sc_ui_render_ctx *ctx,
                              struct sc_point center, int scale) {
    uint64_t rows[SC_UI_TOUCHMAP_WALK_ICON_HEIGHT];
    SDL_memset(rows, 0, sizeof(rows));

    int center_x = SC_UI_TOUCHMAP_WALK_ICON_WIDTH / 2;
    int center_y = SC_UI_TOUCHMAP_WALK_ICON_HEIGHT / 2;
    int offset = (SC_UI_TOUCHMAP_GLYPH_HEIGHT + SC_UI_TOUCHMAP_GLYPH_SPACING)
               / 2 * SC_UI_TOUCHMAP_GLYPH_SCALE_WALK;
    int glyph_offset = offset / scale;
    int half_glyph_width = SC_UI_TOUCHMAP_GLYPH_WIDTH / 2;
    int half_glyph_height = SC_UI_TOUCHMAP_GLYPH_HEIGHT / 2;

    sc_ui_touchmap_compose_glyph_at(rows, SC_UI_TOUCHMAP_WALK_ICON_WIDTH,
                                    SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                    sc_ui_touchmap_glyph_data[
                                        SC_UI_TOUCHMAP_GLYPH_LEFT],
                                    center_x - glyph_offset - half_glyph_width,
                                    center_y - half_glyph_height);
    sc_ui_touchmap_compose_glyph_at(rows, SC_UI_TOUCHMAP_WALK_ICON_WIDTH,
                                    SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                    sc_ui_touchmap_glyph_data[
                                        SC_UI_TOUCHMAP_GLYPH_RIGHT],
                                    center_x + glyph_offset - half_glyph_width,
                                    center_y - half_glyph_height);
    sc_ui_touchmap_compose_glyph_at(rows, SC_UI_TOUCHMAP_WALK_ICON_WIDTH,
                                    SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                    sc_ui_touchmap_glyph_data[
                                        SC_UI_TOUCHMAP_GLYPH_UP],
                                    center_x - half_glyph_width,
                                    center_y - glyph_offset - half_glyph_height);
    sc_ui_touchmap_compose_glyph_at(rows, SC_UI_TOUCHMAP_WALK_ICON_WIDTH,
                                    SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                    sc_ui_touchmap_glyph_data[
                                        SC_UI_TOUCHMAP_GLYPH_DOWN],
                                    center_x - half_glyph_width,
                                    center_y + glyph_offset - half_glyph_height - 1);

    SDL_Rect rect = {
        .w = SC_UI_TOUCHMAP_WALK_ICON_WIDTH * scale,
        .h = SC_UI_TOUCHMAP_WALK_ICON_HEIGHT * scale,
    };
    rect.x = center.x - rect.w / 2;
    rect.y = center.y - rect.h / 2;
    return sc_ui_draw_bitmap_icon_in_rect(ctx, &rect, rows,
                                          SC_UI_TOUCHMAP_WALK_ICON_WIDTH,
                                          SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                          sc_ui_touchmap_color_from_u32(
                                              SC_UI_TOUCHMAP_TEXT_COLOR));
}

static bool
sc_ui_touchmap_layer_render_selection(
        const struct sc_ui_render_ctx *render_ctx,
        const struct sc_gptm_gamepad_touchmap *touchmap,
        const struct sc_touchmap_editor *editor) {
    struct sc_touchmap_editor_selection selection = editor->selection;
    switch (selection.target) {
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER:
            if (!touchmap->has_walk) {
                return true;
            }
            return sc_ui_touchmap_draw_selection_circle(
                render_ctx, touchmap->walk.center, SC_TOUCHMAP_WALK_RADIUS + 4);
        case SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS:
            if (!touchmap->has_walk) {
                return true;
            }
            return sc_ui_touchmap_draw_selection_circle(
                render_ctx, touchmap->walk.center, touchmap->walk.radius + 4);
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER:
        case SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS: {
            int index = selection.button_index;
            if (index < 0 || index >= touchmap->button_cnt) {
                return true;
            }

            const struct sc_gptm_touch_button *btn = &touchmap->buttons[index];
            int32_t radius = selection.target
                                 == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS
                             && btn->radius > 0
                                  ? btn->radius + 4
                                 : SC_TOUCHMAP_BUTTON_RADIUS + 4;
            return sc_ui_touchmap_draw_selection_circle(render_ctx, btn->center,
                                                        radius);
        }
        default:
            return true;
    }
}

static bool
sc_ui_touchmap_layer_render_touchmap(
        const struct sc_ui_touchmap_layer *tm,
        const struct sc_ui_render_ctx *render_ctx) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    const struct sc_gptm_gamepad_touchmap *touchmap = state->map;
    if (!touchmap) {
        return true;
    }

    bool ok = true;
    if (touchmap->has_walk && touchmap->walk.radius > 0) {
        ok &= sc_ui_touchmap_draw_filled_circle(render_ctx,
                                                touchmap->walk.center,
                                                touchmap->walk.radius,
                                                SC_UI_TOUCHMAP_WALK_COLOR);
        ok &= sc_ui_touchmap_draw_circle_outline(render_ctx,
                                                 touchmap->walk.center,
                                                 touchmap->walk.radius,
                                                 SC_UI_TOUCHMAP_TEXT_COLOR,
                                                 false);

        ok &= sc_ui_touchmap_draw_walk_icon(render_ctx, touchmap->walk.center,
                                            SC_UI_TOUCHMAP_GLYPH_SCALE_WALK);
    }

    if (touchmap->has_walk && touchmap->walk.touch_down) {
        ok &= sc_ui_touchmap_draw_filled_circle(render_ctx,
                                                touchmap->walk.current_pos,
                                                SC_UI_TOUCHMAP_WALK_POS_RADIUS,
                                                0xFFFFFFE0);
    }

    for (int i = 0; i < touchmap->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &touchmap->buttons[i];
        bool bound = sc_gptm_touch_button_is_bound(btn);
        uint32_t fill_color = !bound ? SC_UI_TOUCHMAP_UNBOUND_COLOR
                            : btn->is_skill ? SC_UI_TOUCHMAP_SKILL_COLOR
                                            : SC_UI_TOUCHMAP_BUTTON_COLOR;
        uint32_t outline_color = !bound ? 0xFF3434FF
                               : btn->is_skill ? 0xFFFFFFC0 : 0xFFFFFFA0;

        ok &= sc_ui_touchmap_draw_filled_circle(render_ctx, btn->center,
                                                SC_TOUCHMAP_BUTTON_RADIUS,
                                                fill_color);
        ok &= sc_ui_touchmap_draw_circle_outline(render_ctx, btn->center,
                                                 SC_TOUCHMAP_BUTTON_RADIUS,
                                                 outline_color, false);

        if (btn->is_skill && btn->radius > 0 && state->edit_mode) {
            ok &= sc_ui_touchmap_draw_circle_outline(render_ctx, btn->center,
                                                     btn->radius,
                                                     SC_UI_TOUCHMAP_DASH_COLOR,
                                                     true);
        }

        if (btn->touch_down) {
            ok &= sc_ui_touchmap_draw_filled_circle(render_ctx,
                                                    btn->current_pos,
                                                    SC_UI_TOUCHMAP_PRESSED_RADIUS,
                                                    outline_color);
        }

        ok &= sc_ui_touchmap_draw_button_label(render_ctx, btn->center,
                                               sc_ui_touchmap_layer_get_button_label(
                                                   btn->button),
                                               SC_UI_TOUCHMAP_GLYPH_SCALE_BUTTON);
    }

    if (state->edit_mode) {
        ok &= sc_ui_touchmap_layer_render_selection(render_ctx, touchmap,
                                                    &state->editor);
    }

    SDL_SetRenderDrawBlendMode(render_ctx->renderer, SDL_BLENDMODE_NONE);
    return ok;
}

static const char *
sc_ui_touchmap_layer_get_edit_label(const struct sc_touchmap_state *state) {
    return !state->map ? "NEW"
         : state->edit_mode ? "QUIT"
         : "EDIT";
}

static void
sc_ui_touchmap_layer_mark_edited(struct sc_ui_touchmap_layer *tm,
                                 struct sc_ui_input_result *result) {
    tm->touchmap_state->dirty = true;
    tm->touchmap_state->exit_after_save = false;
    result->request_refresh = true;
}

static bool
sc_ui_touchmap_layer_selection_is_walk(
        const struct sc_ui_touchmap_layer *tm) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    if (!state->map || !state->map->has_walk) {
        return false;
    }

    struct sc_touchmap_editor_selection selection = state->editor.selection;
    return selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER
        || selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS;
}

static bool
sc_ui_touchmap_layer_selection_is_button(
        const struct sc_ui_touchmap_layer *tm) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    if (!state->map) {
        return false;
    }

    struct sc_touchmap_editor_selection selection = state->editor.selection;
    return (selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER
            || selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS)
        && selection.button_index >= 0
        && selection.button_index < state->map->button_cnt;
}

static bool
sc_ui_touchmap_layer_delete_selected_control(
        struct sc_ui_touchmap_layer *tm,
        struct sc_ui_input_result *result) {
    struct sc_touchmap_state *state = tm->touchmap_state;
    struct sc_gptm_gamepad_touchmap *map = state->map;
    if (!map) {
        return true;
    }

    if (sc_ui_touchmap_layer_selection_is_walk(tm)) {
        if (sc_gptm_gamepad_touchmap_remove_walk(map)) {
            sc_touchmap_editor_clear_selection(&state->editor);
            sc_ui_touchmap_layer_mark_edited(tm, result);
        }
        return true;
    }

    if (sc_ui_touchmap_layer_selection_is_button(tm)) {
        int old_index = state->editor.selection.button_index;
        int new_index = -1;
        map = sc_gptm_gamepad_touchmap_remove_button(map, old_index,
                                                     &new_index);
        if (map) {
            state->map = map;
            sc_touchmap_editor_select_after_button_remove(&state->editor, map,
                                                          new_index);
            sc_ui_touchmap_layer_mark_edited(tm, result);
        }
        return true;
    }

    return true;
}

static bool
sc_ui_touchmap_layer_capture_binding(struct sc_ui_touchmap_layer *tm,
                                     uint8_t button,
                                     struct sc_ui_input_result *result) {
    struct sc_touchmap_state *state = tm->touchmap_state;
    if (!state->map || !state->edit_mode
            || sc_touchmap_editor_get_mode(&state->editor)
                != SC_TOUCHMAP_EDITOR_MODE_SELECT
            || !sc_ui_touchmap_layer_selection_is_button(tm)) {
        return false;
    }

    int index = state->editor.selection.button_index;
    int new_index = -1;
    if (sc_gptm_gamepad_touchmap_bind_button(state->map, index, button,
                                             &new_index)) {
        sc_touchmap_editor_select_button(&state->editor, new_index);
        sc_ui_touchmap_layer_mark_edited(tm, result);
        return true;
    }

    return false;
}

static void
sc_ui_touchmap_layer_action_edit(void *userdata,
                                 struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    if (!tm->touchmap_state->map) {
        sc_touchmap_state_create_empty(tm->touchmap_state);
    } else {
        sc_touchmap_state_enter_edit_mode(tm->touchmap_state);
    }
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_menu_toggle(void *userdata,
                                        struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->add_menu_open = !tm->touchmap_state->add_menu_open;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_delete(void *userdata,
                                   struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    sc_ui_touchmap_layer_delete_selected_control(tm, result);
}

static void
sc_ui_touchmap_layer_action_quit(void *userdata,
                                 struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    sc_touchmap_state_quit_edit_mode(tm->touchmap_state);
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_button(void *userdata,
                                       struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->add_menu_open = false;
    sc_touchmap_state_add_button_at_center(tm->touchmap_state, false);
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_skill(void *userdata,
                                      struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->add_menu_open = false;
    sc_touchmap_state_add_button_at_center(tm->touchmap_state, true);
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_walk(void *userdata,
                                     struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->add_menu_open = false;
    sc_touchmap_state_add_walk_at_center(tm->touchmap_state);
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_sync_layout(struct sc_ui_touchmap_layer *tm,
                                 const struct sc_ui_geometry *geometry) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);
    static const enum sc_ui_widget_button_variant toolbar_variants[] = {
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DANGER,
    };
    static const enum sc_ui_widget_button_variant menu_variants[] = {
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
    };

    const char *edit_label = sc_ui_touchmap_layer_get_edit_label(state);
    sc_ui_widget_action_button_apply_variant(&tm->edit_button,
                                             SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    tm->edit_button.margin = SC_UI_TOUCHMAP_EDIT_MARGIN;
    sc_ui_widget_action_button_set_label_and_layout_top_right(&tm->edit_button,
                                                              edit_label,
                                                              logical_size);

    sc_ui_widget_action_menu_apply_variants(&tm->action_menu,
                                            toolbar_variants,
                                            menu_variants);

    sc_ui_widget_action_menu_set_spacing(&tm->action_menu,
                                         SC_UI_TOUCHMAP_EDIT_MARGIN,
                                         SC_UI_TOUCHMAP_TOOLBAR_PADDING,
                                         SC_UI_TOUCHMAP_TOOLBAR_GAP,
                                         SC_UI_TOUCHMAP_MENU_GAP);
    sc_ui_widget_action_menu_layout_top_right(&tm->action_menu, logical_size);
    sc_ui_widget_action_menu_set_menu_button_enabled(
        &tm->action_menu, 2, !(state->map && state->map->has_walk));
}

static bool
sc_ui_touchmap_layer_is_visible(const struct sc_ui_touchmap_layer *layer) {
    return layer->touchmap_state
        && (layer->touchmap_state->overlay_enabled
            || layer->touchmap_state->edit_mode);
}

static void
sc_ui_touchmap_layer_sync(struct sc_ui_layer *layer, struct sc_ui_context *ui) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);
    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return;
    }

    sc_ui_touchmap_layer_sync_layout(tm, geometry);
}

static bool
sc_ui_touchmap_layer_handle_edit_key(struct sc_ui_touchmap_layer *tm,
                                     const struct sc_ui_event *event,
                                     struct sc_ui_input_result *result) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    SDL_Keycode keycode = event->data.key.keycode;
    uint16_t mod = event->data.key.mod;

    if (keycode == SDLK_s && sc_touchmap_has_ctrl_modifier()
            && event->type == SC_UI_EVENT_KEY_DOWN
            && !event->data.key.repeat && state->map) {
        sc_touchmap_state_save(tm->touchmap_state,
                               (mod & KMOD_SHIFT) || !state->file);
        result->request_refresh = true;
        return true;
    }

    if (!state->edit_mode) {
        return false;
    }

    if (keycode == SDLK_ESCAPE) {
        bool pending = sc_touchmap_editor_get_mode(&state->editor)
                    != SC_TOUCHMAP_EDITOR_MODE_SELECT
                    || state->add_menu_open;
        if (!pending) {
            return false;
        }

        if (event->type == SC_UI_EVENT_KEY_DOWN && !event->data.key.repeat) {
            sc_touchmap_editor_set_mode(&tm->touchmap_state->editor,
                                        SC_TOUCHMAP_EDITOR_MODE_SELECT);
            tm->touchmap_state->add_menu_open = false;
            result->request_refresh = true;
        }
        return true;
    }

    if (keycode != SDLK_LEFT && keycode != SDLK_RIGHT && keycode != SDLK_UP
            && keycode != SDLK_DOWN) {
        return false;
    }

    if (mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) {
        return false;
    }

    if (event->type != SC_UI_EVENT_KEY_DOWN) {
        return true;
    }

    int32_t step = mod & KMOD_SHIFT ? 10 : 1;
    int32_t dx = 0;
    int32_t dy = 0;
    int32_t radius_delta = 0;
    switch (keycode) {
        case SDLK_LEFT:
            dx = -step;
            radius_delta = -step;
            break;
        case SDLK_RIGHT:
            dx = step;
            radius_delta = step;
            break;
        case SDLK_UP:
            dy = -step;
            radius_delta = step;
            break;
        case SDLK_DOWN:
            dy = step;
            radius_delta = -step;
            break;
        default:
            assert(false);
            return true;
    }

    if (sc_touchmap_editor_nudge_selection(&tm->touchmap_state->editor,
                                           tm->touchmap_state->map, dx, dy,
                                           radius_delta)) {
        sc_ui_touchmap_layer_mark_edited(tm, result);
    }
    return true;
}

static bool
sc_ui_touchmap_layer_handle_gamepad_event(
        struct sc_ui_touchmap_layer *tm,
        const struct sc_ui_event *event,
        struct sc_ui_input_result *result) {
    if (!tm->touchmap_state->edit_mode) {
        return false;
    }

    if (event->type == SC_UI_EVENT_GAMEPAD_BUTTON_DOWN
            && event->data.gamepad_button.pressed) {
        return sc_ui_touchmap_layer_capture_binding(
            tm, event->data.gamepad_button.button, result);
    }

    if (event->type == SC_UI_EVENT_GAMEPAD_AXIS
            && (event->data.gamepad_axis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT
                || event->data.gamepad_axis.axis
                    == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
            && event->data.gamepad_axis.value > SDL_MAX_SINT16 / 2) {
        return sc_ui_touchmap_layer_capture_binding(
            tm, SDL_CONTROLLER_BUTTON_MAX + event->data.gamepad_axis.axis,
            result);
    }

    return false;
}

static struct sc_ui_input_result
sc_ui_touchmap_layer_handle_edit_pointer(
        struct sc_ui_touchmap_layer *tm,
        const struct sc_ui_event *event) {
    struct sc_ui_input_result result = {.consumed = true, .request_refresh = false};
    if (event->type == SC_UI_EVENT_POINTER_WHEEL) {
        return result;
    }

    if (event->type == SC_UI_EVENT_POINTER_MOVE) {
        if (sc_touchmap_editor_is_dragging(&tm->touchmap_state->editor)
                && sc_touchmap_editor_apply_drag(&tm->touchmap_state->editor,
                                                 tm->touchmap_state->map,
                                                 (struct sc_point) {
                                                     .x = event->data.pointer.x,
                                                     .y = event->data.pointer.y,
                                                 })) {
            sc_ui_touchmap_layer_mark_edited(tm, &result);
        }
        return result;
    }

    if (event->data.pointer.button != SDL_BUTTON_LEFT) {
        return result;
    }

    if (event->type == SC_UI_EVENT_POINTER_UP) {
        if (sc_touchmap_editor_is_dragging(&tm->touchmap_state->editor)) {
            sc_touchmap_editor_reset_drag(&tm->touchmap_state->editor);
            result.request_refresh = true;
        }
        return result;
    }

    assert(event->type == SC_UI_EVENT_POINTER_DOWN);
    struct sc_point point = {
        .x = event->data.pointer.x,
        .y = event->data.pointer.y,
    };
    if (sc_touchmap_editor_get_mode(&tm->touchmap_state->editor)
            == SC_TOUCHMAP_EDITOR_MODE_ADD_MENU) {
        sc_touchmap_editor_set_mode(&tm->touchmap_state->editor,
                                    SC_TOUCHMAP_EDITOR_MODE_SELECT);
        tm->touchmap_state->add_menu_open = false;
        result.request_refresh = true;
        return result;
    }

    sc_touchmap_editor_try_start_drag(&tm->touchmap_state->editor,
                                      tm->touchmap_state->map, point);
    result.request_refresh = true;
    return result;
}

static struct sc_ui_input_result
sc_ui_touchmap_layer_handle_event(struct sc_ui_layer *layer,
                                  struct sc_ui_context *ui,
                                  const struct sc_ui_event *event) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_touchmap_state *state = tm->touchmap_state;

    struct sc_ui_input_result result = {false, false};

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return result;
    }

    if (event->type == SC_UI_EVENT_KEY_DOWN || event->type == SC_UI_EVENT_KEY_UP) {
        result.consumed = sc_ui_touchmap_layer_handle_edit_key(tm, event,
                                                                &result);
        return result;
    }

    if (event->type == SC_UI_EVENT_GAMEPAD_AXIS
            || event->type == SC_UI_EVENT_GAMEPAD_BUTTON_DOWN
            || event->type == SC_UI_EVENT_GAMEPAD_BUTTON_UP) {
        result.consumed = sc_ui_touchmap_layer_handle_gamepad_event(tm, event,
                                                                     &result);
        return result;
    }

    if (state->edit_mode && (event->type == SC_UI_EVENT_POINTER_DOWN
                             || event->type == SC_UI_EVENT_POINTER_UP
                             || event->type == SC_UI_EVENT_POINTER_MOVE
                             || event->type == SC_UI_EVENT_POINTER_WHEEL)) {
        struct sc_ui_widget_action_menu_result menu_result =
            sc_ui_widget_action_menu_handle_event(&tm->action_menu, ui, layer,
                                                  event,
                                                  tm->touchmap_state->add_menu_open);
        result = menu_result.input;
        if (!result.consumed && menu_result.clicked_outside) {
            tm->touchmap_state->add_menu_open = false;
            result.request_refresh = true;
        }
        if (result.consumed) {
            return result;
        }

        return sc_ui_touchmap_layer_handle_edit_pointer(tm, event);
    }

    if (!state->edit_mode) {
        result = sc_ui_widget_action_button_handle_event(&tm->edit_button, ui,
                                                         layer, event);
    }

    return result;
}

static bool
sc_ui_touchmap_layer_render(struct sc_ui_layer *layer,
                            const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_touchmap_state *state = tm->touchmap_state;

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return true;
    }

    bool ok = sc_ui_touchmap_layer_render_touchmap(tm, render_ctx);

    if (state->edit_mode) {
        ok &= sc_ui_widget_action_menu_render(&tm->action_menu, render_ctx,
                                              state->add_menu_open);
    } else {
        ok &= sc_ui_widget_action_button_render(&tm->edit_button, render_ctx);
    }

    return ok;
}

void
sc_ui_touchmap_layer_init(struct sc_ui_touchmap_layer *layer,
                          struct sc_input_manager *input_manager,
                          struct sc_touchmap_state *touchmap_state) {
    static const struct sc_ui_layer_ops ops = {
        .sync = sc_ui_touchmap_layer_sync,
        .handle_event = sc_ui_touchmap_layer_handle_event,
        .render = sc_ui_touchmap_layer_render,
        .on_detach = NULL,
    };

    layer->input_manager = input_manager;
    layer->touchmap_state = touchmap_state;
    layer->layer = (struct sc_ui_layer) {
        .ops = &ops,
        .visible = true,
        .enabled = true,
        .z_index = 10,
        .userdata = layer,
    };

    sc_ui_widget_action_button_init(&layer->edit_button,
                                    sc_ui_id_from_u32(layer, 0), "EDIT",
                                    sc_ui_touchmap_layer_action_edit, layer);

    sc_ui_widget_button_init_default(&layer->toolbar_buttons[0],
                                     sc_ui_id_from_u32(layer, 1), "ADD");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[1],
                                     sc_ui_id_from_u32(layer, 2), "DEL");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[2],
                                     sc_ui_id_from_u32(layer, 3), "QUIT");

    sc_ui_widget_button_init_default(&layer->add_menu_items[0],
                                     sc_ui_id_from_u32(layer, 4), "BUTTON");
    sc_ui_widget_button_init_default(&layer->add_menu_items[1],
                                     sc_ui_id_from_u32(layer, 5), "SKILL");
    sc_ui_widget_button_init_default(&layer->add_menu_items[2],
                                     sc_ui_id_from_u32(layer, 6), "WALK");

    struct sc_ui_widget_button *toolbar_buttons[] = {
        &layer->toolbar_buttons[0],
        &layer->toolbar_buttons[1],
        &layer->toolbar_buttons[2],
    };
    static const sc_ui_widget_action_handler toolbar_actions[] = {
        sc_ui_touchmap_layer_action_menu_toggle,
        sc_ui_touchmap_layer_action_delete,
        sc_ui_touchmap_layer_action_quit,
    };
    struct sc_ui_widget_button *menu_buttons[] = {
        &layer->add_menu_items[0],
        &layer->add_menu_items[1],
        &layer->add_menu_items[2],
    };
    static const sc_ui_widget_action_handler menu_actions[] = {
        sc_ui_touchmap_layer_action_add_button,
        sc_ui_touchmap_layer_action_add_skill,
        sc_ui_touchmap_layer_action_add_walk,
    };
    sc_ui_widget_action_menu_init(&layer->action_menu, toolbar_buttons,
                                  ARRAY_LEN(toolbar_buttons), toolbar_actions,
                                  menu_buttons, ARRAY_LEN(menu_buttons),
                                  menu_actions, layer);
}
