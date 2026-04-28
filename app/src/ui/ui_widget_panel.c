#include "ui_widget_panel.h"

void
sc_ui_widget_panel_init(struct sc_ui_widget_panel *panel,
                        const SDL_Rect *rect,
                        const struct sc_ui_widget_panel_style *style) {
    panel->rect = *rect;
    panel->style = *style;
}

SDL_Rect
sc_ui_widget_panel_get_content_rect(const struct sc_ui_widget_panel *panel) {
    int padding = panel->style.padding;
    return (SDL_Rect) {
        .x = panel->rect.x + padding,
        .y = panel->rect.y + padding,
        .w = panel->rect.w - 2 * padding,
        .h = panel->rect.h - 2 * padding,
    };
}

bool
sc_ui_widget_panel_render(const struct sc_ui_widget_panel *panel,
                          const struct sc_ui_render_ctx *render_ctx) {
    bool ok = sc_ui_draw_fill_rect(render_ctx, &panel->rect,
                                   panel->style.fill_color);
    ok &= sc_ui_draw_rect_border(render_ctx, &panel->rect,
                                 panel->style.border_color);
    return ok;
}
