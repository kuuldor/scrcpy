#ifndef SC_UI_WIDGET_MENU_H
#define SC_UI_WIDGET_MENU_H

#include "common.h"

#include <SDL2/SDL_rect.h>

#include "ui_widget_panel.h"

struct sc_ui_widget_menu_style {
    struct sc_ui_widget_panel_style panel;
    int item_width;
    int item_height;
    int item_gap;
};

struct sc_ui_widget_menu {
    struct sc_ui_widget_panel panel;
    struct sc_ui_widget_menu_style style;
};

void
sc_ui_widget_menu_init(struct sc_ui_widget_menu *menu, const SDL_Rect *rect,
                       const struct sc_ui_widget_menu_style *style);

int32_t
sc_ui_widget_menu_total_height(const struct sc_ui_widget_menu *menu,
                               int item_count);

SDL_Rect
sc_ui_widget_menu_get_item_rect(const struct sc_ui_widget_menu *menu,
                                int item_index);

bool
sc_ui_widget_menu_render(const struct sc_ui_widget_menu *menu,
                         const struct sc_ui_render_ctx *render_ctx);

#endif
