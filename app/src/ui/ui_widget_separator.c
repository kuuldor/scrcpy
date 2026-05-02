#include "ui_widget_separator.h"

void
sc_ui_widget_separator_init(struct sc_ui_widget_separator *separator,
                            const SDL_Rect *rect,
                            const struct sc_ui_widget_separator_style *style) {
    separator->rect = *rect;
    separator->style = *style;
}

bool
sc_ui_widget_separator_render(const struct sc_ui_widget_separator *separator,
                              const struct sc_ui_render_ctx *render_ctx) {
    SDL_Rect rect = separator->rect;
    int inset = separator->style.inset;
    if (rect.w > 2 * inset) {
        rect.x += inset;
        rect.w -= 2 * inset;
    }
    if (rect.h > separator->style.thickness) {
        rect.y += (rect.h - separator->style.thickness) / 2;
        rect.h = separator->style.thickness;
    }
    return sc_ui_draw_fill_rect(render_ctx, &rect, separator->style.color);
}
