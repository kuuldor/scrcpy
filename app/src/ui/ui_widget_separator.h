#ifndef SC_UI_WIDGET_SEPARATOR_H
#define SC_UI_WIDGET_SEPARATOR_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_draw.h"

struct sc_ui_widget_separator_style {
    struct sc_ui_color color;
    int thickness;
    int inset;
};

struct sc_ui_widget_separator {
    SDL_Rect rect;
    struct sc_ui_widget_separator_style style;
};

void
sc_ui_widget_separator_init(struct sc_ui_widget_separator *separator,
                            const SDL_Rect *rect,
                            const struct sc_ui_widget_separator_style *style);

bool
sc_ui_widget_separator_render(const struct sc_ui_widget_separator *separator,
                              const struct sc_ui_render_ctx *render_ctx);

#endif
