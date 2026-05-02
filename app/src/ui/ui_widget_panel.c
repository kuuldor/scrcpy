#include "ui_widget_panel.h"

struct sc_ui_widget_panel_style
sc_ui_widget_panel_style_default(void) {
    return (struct sc_ui_widget_panel_style) {
        .fill_color = sc_ui_color_rgba(0x10, 0x10, 0x16, 0xD8),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
        .padding = 6,
    };
}

void
sc_ui_widget_panel_init(struct sc_ui_widget_panel *panel,
                        const SDL_Rect *rect,
                        const struct sc_ui_widget_panel_style *style) {
    panel->rect = *rect;
    panel->style = *style;
}

void
sc_ui_widget_panel_init_default(struct sc_ui_widget_panel *panel) {
    SDL_Rect rect = {0, 0, 0, 0};
    struct sc_ui_widget_panel_style style = sc_ui_widget_panel_style_default();
    sc_ui_widget_panel_init(panel, &rect, &style);
}

void
sc_ui_widget_panel_place_top_right(struct sc_ui_widget_panel *panel,
                                   struct sc_size bounds, int margin) {
    panel->rect.x = bounds.width - panel->rect.w - margin;
    panel->rect.y = margin;
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
