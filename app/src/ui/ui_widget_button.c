#include "ui_widget_button.h"

void
sc_ui_widget_button_init(struct sc_ui_widget_button *button, sc_ui_id id,
                         const SDL_Rect *rect, const char *label,
                         const struct sc_ui_widget_button_style *style) {
    sc_ui_button_init(&button->state, id);
    button->rect = *rect;
    button->label = label;
    button->style = *style;
}

void
sc_ui_widget_button_reset(struct sc_ui_widget_button *button,
                          struct sc_ui_context *ui,
                          struct sc_ui_layer *layer) {
    sc_ui_button_reset(&button->state, ui, layer);
}

struct sc_ui_button_result
sc_ui_widget_button_handle_event(struct sc_ui_widget_button *button,
                                 struct sc_ui_context *ui,
                                 struct sc_ui_layer *layer,
                                 const struct sc_ui_event *event) {
    return sc_ui_button_handle_event(&button->state, ui, layer, &button->rect,
                                     event);
}

bool
sc_ui_widget_button_render(const struct sc_ui_widget_button *button,
                           const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_color fill = button->state.pressed
                            ? button->style.fill_pressed_color
                            : button->state.hovered
                                ? button->style.fill_hover_color
                                : button->style.fill_color;
    struct sc_ui_color border = button->state.pressed
                              ? button->style.border_pressed_color
                              : button->style.border_color;

    bool ok = sc_ui_draw_fill_rect(render_ctx, &button->rect, fill);
    ok &= sc_ui_draw_rect_border(render_ctx, &button->rect, border);
    ok &= sc_ui_text_draw_in_rect(render_ctx, &button->rect, button->label,
                                  &button->style.text_style,
                                  SC_UI_TEXT_ALIGN_CENTER);
    return ok;
}
