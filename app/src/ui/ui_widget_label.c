#include "ui_widget_label.h"

bool
sc_ui_widget_label_render(const struct sc_ui_widget_label *label,
                         const struct sc_ui_render_ctx *render_ctx) {
    return sc_ui_text_draw(render_ctx, label->rect.x, label->rect.y,
                           label->text, &label->style.text_style);
}

void
sc_ui_widget_label_init(struct sc_ui_widget_label *label,
                        const SDL_Rect *rect, const char *text,
                        const struct sc_ui_widget_label_style *style) {
    label->rect = *rect;
    label->text = text;
    label->style = *style;
}

int32_t
sc_ui_widget_label_width_for_text(const char *text,
                                  const struct sc_ui_widget_label_style *style) {
    struct sc_ui_text_metrics metrics = sc_ui_text_measure(text, &style->text_style);
    return metrics.width;
}

int32_t
sc_ui_widget_label_height_for_style(
    const struct sc_ui_widget_label_style *style) {
    struct sc_ui_text_metrics metrics = sc_ui_text_measure("M", &style->text_style);
    return metrics.height;
}