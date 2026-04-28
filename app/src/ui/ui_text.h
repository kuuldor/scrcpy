#ifndef SC_UI_TEXT_H
#define SC_UI_TEXT_H

#include "common.h"

#include <stddef.h>

#include <SDL2/SDL_rect.h>

#include "ui_draw.h"

enum sc_ui_text_align {
    SC_UI_TEXT_ALIGN_LEFT,
    SC_UI_TEXT_ALIGN_CENTER,
    SC_UI_TEXT_ALIGN_RIGHT,
};

struct sc_ui_text_style {
    struct sc_ui_color color;
    int scale;
    int tracking;
};

struct sc_ui_text_metrics {
    int width;
    int height;
};

struct sc_ui_text_metrics
sc_ui_text_measure(const char *text, const struct sc_ui_text_style *style);

bool
sc_ui_text_draw(const struct sc_ui_render_ctx *render_ctx,
                int32_t x, int32_t y, const char *text,
                const struct sc_ui_text_style *style);

bool
sc_ui_text_draw_in_rect(const struct sc_ui_render_ctx *render_ctx,
                        const SDL_Rect *rect, const char *text,
                        const struct sc_ui_text_style *style,
                        enum sc_ui_text_align align);

#endif
