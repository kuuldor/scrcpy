#include "ui_touchmap_layer.h"

#include <SDL2/SDL.h>

#include "input_manager.h"
#include "ui/ui_draw.h"
#include "ui/ui_context.h"
#include "ui/ui_geom.h"
#include "ui/ui_id.h"
#include "ui/ui_widget_circle_button.h"
#include "touchmap/touchmap_state.h"

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
#define SC_UI_TOUCHMAP_WALK_ICON_SIZE 144
#define SC_UI_TOUCHMAP_GLYPH_SPACING (-4)
#define SC_UI_TOUCHMAP_GLYPH_WIDTH 24
#define SC_UI_TOUCHMAP_GLYPH_HEIGHT 24
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
sc_ui_touchmap_draw_selection_circle(const struct sc_ui_render_ctx *ctx,
                                     struct sc_point center, int32_t radius) {
    for (int32_t offset = 0; offset < 3; ++offset) {
        if (!sc_ui_draw_circle_outline(
                ctx, center, radius + offset,
                sc_ui_touchmap_color_from_u32(SC_UI_TOUCHMAP_SELECTION_COLOR),
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
sc_ui_touchmap_button_is_checked(const struct sc_touchmap_state *state,
                                 int index) {
    if (!state->edit_mode || !state->map) {
        return false;
    }

    struct sc_touchmap_editor_selection selection = state->editor.selection;
    return (selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER
            || selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS)
        && selection.button_index == index;
}

static bool
sc_ui_touchmap_walk_is_checked(const struct sc_touchmap_state *state) {
    if (!state->edit_mode || !state->map || !state->map->has_walk) {
        return false;
    }

    struct sc_touchmap_editor_selection selection = state->editor.selection;
    return selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER
        || selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS;
}

static void
sc_ui_touchmap_apply_circle_colors(struct sc_ui_widget_circle_button *widget,
                                   uint32_t fill_color,
                                   uint32_t outline_color) {
    widget->style.fill_color = sc_ui_touchmap_color_from_u32(fill_color);
    widget->style.fill_hover_color = widget->style.fill_color;
    widget->style.fill_pressed_color = widget->style.fill_color;
    widget->style.fill_checked_color = widget->style.fill_color;
    widget->style.fill_disabled_color = widget->style.fill_color;
    widget->style.outline_color = sc_ui_touchmap_color_from_u32(outline_color);
    widget->style.outline_hover_color = sc_ui_touchmap_color_from_u32(0xFFFFFFFF);
    widget->style.outline_pressed_color =
        sc_ui_touchmap_color_from_u32(0xFFFFFFFF);
    widget->style.outline_checked_color =
        sc_ui_touchmap_color_from_u32(SC_UI_TOUCHMAP_SELECTION_COLOR);
    widget->style.outline_disabled_color = widget->style.outline_color;
    widget->style.icon_color = sc_ui_touchmap_color_from_u32(
        SC_UI_TOUCHMAP_TEXT_COLOR);
}

static struct sc_ui_widget_circle_button
sc_ui_touchmap_make_button_widget(const struct sc_ui_touchmap_layer *tm,
                                  const struct sc_gptm_touch_button *btn,
                                  int index) {
    bool bound = sc_gptm_touch_button_is_bound(btn);
    uint32_t fill_color = !bound ? SC_UI_TOUCHMAP_UNBOUND_COLOR
                        : btn->is_skill ? SC_UI_TOUCHMAP_SKILL_COLOR
                                        : SC_UI_TOUCHMAP_BUTTON_COLOR;
    uint32_t outline_color = !bound ? 0xFF3434FF
                           : btn->is_skill ? 0xFFFFFFC0 : 0xFFFFFFA0;

    struct sc_ui_widget_circle_button widget;
    sc_ui_widget_circle_button_init(&widget,
                                    sc_ui_id_from_u32(tm, 1000 + index),
                                    btn->center, SC_TOUCHMAP_BUTTON_RADIUS);
    sc_ui_touchmap_apply_circle_colors(&widget, fill_color, outline_color);
    widget.checked = sc_ui_touchmap_button_is_checked(tm->touchmap_state, index);

    if (btn->is_skill && btn->radius > 0 && tm->touchmap_state->edit_mode) {
        widget.outer_outline.enabled = true;
        widget.outer_outline.radius = btn->radius;
        widget.outer_outline.color =
            sc_ui_touchmap_color_from_u32(SC_UI_TOUCHMAP_DASH_COLOR);
        widget.outer_outline.dashed = true;
    }

    if (btn->touch_down) {
        widget.marker.enabled = true;
        widget.marker.center = btn->current_pos;
        widget.marker.radius = SC_UI_TOUCHMAP_PRESSED_RADIUS;
        widget.marker.color = sc_ui_touchmap_color_from_u32(outline_color);
    }

    return widget;
}

static struct sc_ui_widget_circle_button
sc_ui_touchmap_make_walk_widget(const struct sc_ui_touchmap_layer *tm,
                                const struct sc_gptm_walk_control *walk) {
    struct sc_ui_widget_circle_button widget;
    sc_ui_widget_circle_button_init(&widget, sc_ui_id_from_u32(tm, 900),
                                    walk->center, walk->radius);
    sc_ui_touchmap_apply_circle_colors(&widget, SC_UI_TOUCHMAP_WALK_COLOR,
                                       SC_UI_TOUCHMAP_TEXT_COLOR);
    widget.checked = sc_ui_touchmap_walk_is_checked(tm->touchmap_state);
    widget.icon.width = SC_UI_TOUCHMAP_WALK_ICON_WIDTH;
    widget.icon.height = SC_UI_TOUCHMAP_WALK_ICON_HEIGHT;
    widget.icon.size = SC_UI_TOUCHMAP_WALK_ICON_SIZE;

    if (walk->touch_down) {
        widget.marker.enabled = true;
        widget.marker.center = walk->current_pos;
        widget.marker.radius = SC_UI_TOUCHMAP_WALK_POS_RADIUS;
        widget.marker.color = sc_ui_touchmap_color_from_u32(0xFFFFFFE0);
    }

    return widget;
}

static uint64_t *
sc_ui_touchmap_compose_walk_icon(void) {
    uint64_t *rows = SDL_calloc(SC_UI_TOUCHMAP_WALK_ICON_HEIGHT,
                                sizeof(*rows));
    if (!rows) {
        return NULL;
    }

    int center_x = SC_UI_TOUCHMAP_WALK_ICON_WIDTH / 2;
    int center_y = SC_UI_TOUCHMAP_WALK_ICON_HEIGHT / 2;
    int glyph_offset = (SC_UI_TOUCHMAP_GLYPH_HEIGHT
                      + SC_UI_TOUCHMAP_GLYPH_SPACING) / 2;
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

    return rows;
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
        struct sc_ui_widget_circle_button widget =
            sc_ui_touchmap_make_walk_widget(tm, &touchmap->walk);

        uint64_t *rows = sc_ui_touchmap_compose_walk_icon();
        widget.icon.rows = rows;

        ok &= sc_ui_widget_circle_button_render(&widget, render_ctx);
        SDL_free(rows);
    }

    for (int i = 0; i < touchmap->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &touchmap->buttons[i];
        struct sc_ui_widget_circle_button widget =
            sc_ui_touchmap_make_button_widget(tm, btn, i);

        uint64_t rows[SC_UI_TOUCHMAP_GLYPH_HEIGHT];
        int width;
        if (sc_ui_touchmap_compose_label_icon(
                sc_ui_touchmap_layer_get_button_label(btn->button), rows,
                &width)) {
            widget.icon.rows = rows;
            widget.icon.width = width;
            widget.icon.height = SC_UI_TOUCHMAP_GLYPH_HEIGHT;
            widget.icon.size = width;
        }

        ok &= sc_ui_widget_circle_button_render(&widget, render_ctx);
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
    }
    sc_touchmap_state_enter_edit_mode(tm->touchmap_state);
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
    sc_ui_widget_button_apply_variant(&tm->edit_button,
                                      SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    sc_ui_widget_button_set_label(&tm->edit_button, edit_label);
    sc_ui_widget_button_fit_to_content(&tm->edit_button);
    sc_ui_widget_button_place_top_right(&tm->edit_button, logical_size,
                                        SC_UI_TOUCHMAP_EDIT_MARGIN);

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

static struct sc_ui_button_result
sc_ui_touchmap_layer_handle_button_widgets_event(
        struct sc_ui_touchmap_layer *tm, struct sc_ui_context *ui,
        const struct sc_ui_event *event, int *out_index) {
    struct sc_ui_button_result result = {
        .input = {false, false},
        .action = SC_UI_BUTTON_ACTION_NONE,
    };
    *out_index = -1;

    struct sc_gptm_gamepad_touchmap *map = tm->touchmap_state->map;
    if (!map) {
        return result;
    }

    for (int i = 0; i < map->button_cnt; ++i) {
        struct sc_ui_widget_circle_button widget =
            sc_ui_touchmap_make_button_widget(tm, &map->buttons[i], i);
        result = sc_ui_widget_circle_button_handle_event(&widget, ui,
                                                         &tm->layer, event);
        if (result.input.consumed) {
            *out_index = i;
            return result;
        }
    }

    return result;
}

static struct sc_ui_button_result
sc_ui_touchmap_layer_handle_walk_widget_event(
        struct sc_ui_touchmap_layer *tm, struct sc_ui_context *ui,
        const struct sc_ui_event *event) {
    struct sc_ui_button_result result = {
        .input = {false, false},
        .action = SC_UI_BUTTON_ACTION_NONE,
    };

    struct sc_gptm_gamepad_touchmap *map = tm->touchmap_state->map;
    if (!map || !map->has_walk || map->walk.radius <= 0) {
        return result;
    }

    struct sc_ui_widget_circle_button widget =
        sc_ui_touchmap_make_walk_widget(tm, &map->walk);
    return sc_ui_widget_circle_button_handle_event(&widget, ui, &tm->layer,
                                                   event);
}

static void
sc_ui_touchmap_layer_handle_widget_release(
        struct sc_ui_touchmap_layer *tm, struct sc_ui_context *ui,
        const struct sc_ui_event *event, struct sc_ui_input_result *result) {
    int button_index;
    struct sc_ui_button_result button_result =
        sc_ui_touchmap_layer_handle_button_widgets_event(
            tm, ui, event, &button_index);
    (void) button_index;
    result->request_refresh |= button_result.input.request_refresh;

    if (button_result.input.consumed) {
        return;
    }

    struct sc_ui_button_result walk_result =
        sc_ui_touchmap_layer_handle_walk_widget_event(tm, ui, event);
    result->request_refresh |= walk_result.input.request_refresh;
}

static void
sc_ui_touchmap_layer_handle_widget_press(
        struct sc_ui_touchmap_layer *tm, struct sc_ui_context *ui,
        const struct sc_ui_event *event, struct sc_ui_input_result *result) {
    int button_index;
    struct sc_ui_button_result button_result =
        sc_ui_touchmap_layer_handle_button_widgets_event(
            tm, ui, event, &button_index);
    result->request_refresh |= button_result.input.request_refresh;

    if (button_result.action == SC_UI_BUTTON_ACTION_PRESS && button_index >= 0) {
        sc_touchmap_editor_select_button(&tm->touchmap_state->editor,
                                         button_index);
        result->request_refresh = true;
        return;
    }

    if (button_result.input.consumed) {
        return;
    }

    struct sc_ui_button_result walk_result =
        sc_ui_touchmap_layer_handle_walk_widget_event(tm, ui, event);
    result->request_refresh |= walk_result.input.request_refresh;
    if (walk_result.action == SC_UI_BUTTON_ACTION_PRESS) {
        sc_touchmap_editor_select_walk(&tm->touchmap_state->editor);
        result->request_refresh = true;
    }
}

static struct sc_ui_input_result
sc_ui_touchmap_layer_handle_edit_pointer(
        struct sc_ui_touchmap_layer *tm, struct sc_ui_context *ui,
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
        if (!sc_touchmap_editor_is_dragging(&tm->touchmap_state->editor)) {
            int button_index;
            struct sc_ui_button_result button_result =
                sc_ui_touchmap_layer_handle_button_widgets_event(
                    tm, ui, event, &button_index);
            (void) button_index;
            result.request_refresh |= button_result.input.request_refresh;
            if (!button_result.input.consumed) {
                struct sc_ui_button_result walk_result =
                    sc_ui_touchmap_layer_handle_walk_widget_event(
                        tm, ui, event);
                result.request_refresh |= walk_result.input.request_refresh;
            }
        }
        return result;
    }

    if (event->data.pointer.button != SDL_BUTTON_LEFT) {
        return result;
    }

    if (event->type == SC_UI_EVENT_POINTER_UP) {
        bool was_dragging =
            sc_touchmap_editor_is_dragging(&tm->touchmap_state->editor);
        sc_ui_touchmap_layer_handle_widget_release(tm, ui, event, &result);

        if (was_dragging) {
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

    sc_ui_touchmap_layer_handle_widget_press(tm, ui, event, &result);

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

        return sc_ui_touchmap_layer_handle_edit_pointer(tm, ui, event);
    }

    if (!state->edit_mode) {
        struct sc_ui_button_result button_result =
            sc_ui_widget_button_handle_event(&tm->edit_button, ui,
                                             layer, event);
        result = button_result.input;
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
        ok &= sc_ui_widget_button_render(&tm->edit_button, render_ctx);
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

    sc_ui_widget_button_init_default(&layer->edit_button,
                                     sc_ui_id_from_u32(layer, 0), "EDIT");
    layer->edit_button.handler = sc_ui_touchmap_layer_action_edit;
    layer->edit_button.handler_userdata = layer;

    sc_ui_widget_button_init_default(&layer->toolbar_buttons[0],
                                     sc_ui_id_from_u32(layer, 1), "ADD");
    layer->toolbar_buttons[0].handler = sc_ui_touchmap_layer_action_menu_toggle;
    layer->toolbar_buttons[0].handler_userdata = layer;
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[1],
                                     sc_ui_id_from_u32(layer, 2), "DEL");
    layer->toolbar_buttons[1].handler = sc_ui_touchmap_layer_action_delete;
    layer->toolbar_buttons[1].handler_userdata = layer;
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[2],
                                     sc_ui_id_from_u32(layer, 3), "QUIT");
    layer->toolbar_buttons[2].handler = sc_ui_touchmap_layer_action_quit;
    layer->toolbar_buttons[2].handler_userdata = layer;

    sc_ui_widget_button_init_default(&layer->add_menu_items[0],
                                     sc_ui_id_from_u32(layer, 4), "BUTTON");
    layer->add_menu_items[0].handler = sc_ui_touchmap_layer_action_add_button;
    layer->add_menu_items[0].handler_userdata = layer;
    sc_ui_widget_button_init_default(&layer->add_menu_items[1],
                                     sc_ui_id_from_u32(layer, 5), "SKILL");
    layer->add_menu_items[1].handler = sc_ui_touchmap_layer_action_add_skill;
    layer->add_menu_items[1].handler_userdata = layer;
    sc_ui_widget_button_init_default(&layer->add_menu_items[2],
                                     sc_ui_id_from_u32(layer, 6), "WALK");
    layer->add_menu_items[2].handler = sc_ui_touchmap_layer_action_add_walk;
    layer->add_menu_items[2].handler_userdata = layer;

    struct sc_ui_widget_button *toolbar_buttons[] = {
        &layer->toolbar_buttons[0],
        &layer->toolbar_buttons[1],
        &layer->toolbar_buttons[2],
    };
    struct sc_ui_widget_button *menu_buttons[] = {
        &layer->add_menu_items[0],
        &layer->add_menu_items[1],
        &layer->add_menu_items[2],
    };
    sc_ui_widget_action_menu_init(&layer->action_menu, toolbar_buttons,
                                  ARRAY_LEN(toolbar_buttons),
                                  menu_buttons, ARRAY_LEN(menu_buttons),
                                  layer);
}
