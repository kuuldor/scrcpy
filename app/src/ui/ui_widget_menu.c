#include "ui_widget_menu.h"

static SDL_Rect
sc_ui_widget_menu_get_content_rect(const struct sc_ui_widget_menu *menu) {
    return sc_ui_widget_panel_get_content_rect(&menu->panel);
}

void
sc_ui_widget_menu_init(struct sc_ui_widget_menu *menu, const SDL_Rect *rect,
                       const struct sc_ui_widget_menu_style *style) {
    menu->style = *style;
    sc_ui_widget_panel_init(&menu->panel, rect, &style->panel);
}

int32_t
sc_ui_widget_menu_total_height(const struct sc_ui_widget_menu *menu,
                               int item_count) {
    if (item_count <= 0) {
        return 0;
    }
    return item_count * menu->style.item_height
         + (item_count - 1) * menu->style.item_gap
         + 2 * menu->style.panel.padding;
}

SDL_Rect
sc_ui_widget_menu_get_item_rect(const struct sc_ui_widget_menu *menu,
                                int item_index) {
    SDL_Rect content = sc_ui_widget_menu_get_content_rect(menu);
    return (SDL_Rect) {
        .x = content.x,
        .y = content.y + item_index * (menu->style.item_height
                                     + menu->style.item_gap),
        .w = menu->style.item_width,
        .h = menu->style.item_height,
    };
}

bool
sc_ui_widget_menu_render(const struct sc_ui_widget_menu *menu,
                         const struct sc_ui_render_ctx *render_ctx) {
    return sc_ui_widget_panel_render(&menu->panel, render_ctx);
}
