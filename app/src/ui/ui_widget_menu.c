#include "ui_widget_menu.h"

static SDL_Rect
sc_ui_widget_menu_get_content_rect(const struct sc_ui_widget_menu *menu) {
    return sc_ui_widget_panel_get_content_rect(&menu->panel);
}

struct sc_ui_widget_menu_style
sc_ui_widget_menu_style_default(void) {
    struct sc_ui_widget_panel_style panel = sc_ui_widget_panel_style_default();
    return (struct sc_ui_widget_menu_style) {
        .panel = panel,
        .item_width = 0,
        .item_height = 0,
        .item_gap = 6,
    };
}

void
sc_ui_widget_menu_init(struct sc_ui_widget_menu *menu, const SDL_Rect *rect,
                       const struct sc_ui_widget_menu_style *style) {
    menu->style = *style;
    sc_ui_widget_panel_init(&menu->panel, rect, &style->panel);
}

void
sc_ui_widget_menu_init_default(struct sc_ui_widget_menu *menu) {
    SDL_Rect rect = {0, 0, 0, 0};
    struct sc_ui_widget_menu_style style = sc_ui_widget_menu_style_default();
    sc_ui_widget_menu_init(menu, &rect, &style);
}

void
sc_ui_widget_menu_fit_panel(struct sc_ui_widget_menu *menu, int item_count) {
    menu->panel.rect.w = menu->style.item_width + 2 * menu->style.panel.padding;
    menu->panel.rect.h = sc_ui_widget_menu_total_height(menu, item_count);
}

void
sc_ui_widget_menu_place_below(struct sc_ui_widget_menu *menu,
                              const SDL_Rect *anchor, int gap) {
    menu->panel.rect.x = anchor->x;
    menu->panel.rect.y = anchor->y + anchor->h + gap;
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
