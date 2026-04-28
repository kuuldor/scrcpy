#include "ui_widget_toolbar.h"

static struct sc_ui_row_layout
sc_ui_widget_toolbar_get_layout(const struct sc_ui_widget_toolbar *toolbar) {
    SDL_Rect content = sc_ui_widget_panel_get_content_rect(&toolbar->panel);
    return sc_ui_row_layout_make(content.x, content.y, toolbar->style.item_width,
                                 toolbar->style.item_height,
                                 toolbar->style.item_gap);
}

void
sc_ui_widget_toolbar_init(struct sc_ui_widget_toolbar *toolbar,
                          const SDL_Rect *rect,
                          const struct sc_ui_widget_toolbar_style *style) {
    toolbar->style = *style;
    sc_ui_widget_panel_init(&toolbar->panel, rect, &style->panel);
}

SDL_Rect
sc_ui_widget_toolbar_get_rect(const struct sc_ui_widget_toolbar *toolbar,
                              int item_index) {
    struct sc_ui_row_layout layout = sc_ui_widget_toolbar_get_layout(toolbar);
    return sc_ui_row_layout_get_item_rect(&layout, item_index);
}

int32_t
sc_ui_widget_toolbar_total_width(const struct sc_ui_widget_toolbar *toolbar,
                                 int item_count) {
    struct sc_ui_row_layout layout = {
        .x = 0,
        .y = 0,
        .item_width = toolbar->style.item_width,
        .item_height = toolbar->style.item_height,
        .gap = toolbar->style.item_gap,
    };
    return sc_ui_row_layout_total_width(&layout, item_count)
         + 2 * toolbar->style.panel.padding;
}

bool
sc_ui_widget_toolbar_render(const struct sc_ui_widget_toolbar *toolbar,
                            const struct sc_ui_render_ctx *render_ctx) {
    return sc_ui_widget_panel_render(&toolbar->panel, render_ctx);
}
