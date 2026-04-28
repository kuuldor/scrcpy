#ifndef SC_UI_WIDGET_PANEL_H
#define SC_UI_WIDGET_PANEL_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_draw.h"

struct sc_ui_widget_panel_style {
    struct sc_ui_color fill_color;
    struct sc_ui_color border_color;
    int padding;
};

struct sc_ui_widget_panel {
    SDL_Rect rect;
    struct sc_ui_widget_panel_style style;
};

void
sc_ui_widget_panel_init(struct sc_ui_widget_panel *panel,
                        const SDL_Rect *rect,
                        const struct sc_ui_widget_panel_style *style);

SDL_Rect
sc_ui_widget_panel_get_content_rect(const struct sc_ui_widget_panel *panel);

bool
sc_ui_widget_panel_render(const struct sc_ui_widget_panel *panel,
                          const struct sc_ui_render_ctx *render_ctx);

#endif
