#ifndef SC_UI_DRAW_H
#define SC_UI_DRAW_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL_rect.h>

#include "ui_render.h"

struct sc_ui_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

static inline struct sc_ui_color
sc_ui_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (struct sc_ui_color) {
        .r = r,
        .g = g,
        .b = b,
        .a = a,
    };
}

bool
sc_ui_draw_fill_rect(const struct sc_ui_render_ctx *render_ctx,
                     const SDL_Rect *rect, struct sc_ui_color color);

bool
sc_ui_draw_rect_border(const struct sc_ui_render_ctx *render_ctx,
                       const SDL_Rect *rect, struct sc_ui_color color);

bool
sc_ui_draw_bitmap_icon(const struct sc_ui_render_ctx *render_ctx,
                       int32_t center_x, int32_t center_y,
                       const uint64_t *rows, int width, int height, int scale,
                       struct sc_ui_color color);

bool
sc_ui_draw_bitmap_icon_in_rect(const struct sc_ui_render_ctx *render_ctx,
                               const SDL_Rect *rect, const uint64_t *rows,
                               int width, int height, struct sc_ui_color color);

#endif
