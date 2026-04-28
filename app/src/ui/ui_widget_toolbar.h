#ifndef SC_UI_WIDGET_TOOLBAR_H
#define SC_UI_WIDGET_TOOLBAR_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_layout.h"
#include "ui_widget_panel.h"

struct sc_ui_widget_toolbar_style {
    struct sc_ui_widget_panel_style panel;
    int item_width;
    int item_height;
    int item_gap;
};

struct sc_ui_widget_toolbar {
    struct sc_ui_widget_panel panel;
    struct sc_ui_widget_toolbar_style style;
};

void
sc_ui_widget_toolbar_init(struct sc_ui_widget_toolbar *toolbar,
                          const SDL_Rect *rect,
                          const struct sc_ui_widget_toolbar_style *style);

SDL_Rect
sc_ui_widget_toolbar_get_rect(const struct sc_ui_widget_toolbar *toolbar,
                              int item_index);

int32_t
sc_ui_widget_toolbar_total_width(const struct sc_ui_widget_toolbar *toolbar,
                                 int item_count);

bool
sc_ui_widget_toolbar_render(const struct sc_ui_widget_toolbar *toolbar,
                            const struct sc_ui_render_ctx *render_ctx);

#endif
