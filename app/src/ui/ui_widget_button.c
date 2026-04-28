#include "ui_widget_button.h"

SDL_Rect
sc_ui_widget_button_get_content_rect(const struct sc_ui_widget_button *button) {
    return (SDL_Rect) {
        .x = button->rect.x + button->style.padding_h,
        .y = button->rect.y + button->style.padding_v,
        .w = button->rect.w - 2 * button->style.padding_h,
        .h = button->rect.h - 2 * button->style.padding_v,
    };
}

int32_t
sc_ui_widget_button_width_for_label(const char *label,
                                    const struct sc_ui_widget_button_style *style) {
    struct sc_ui_text_metrics metrics =
        sc_ui_text_measure(label, &style->text_style);
    return metrics.width + 2 * style->padding_h;
}

int32_t
sc_ui_widget_button_height_for_style(
    const struct sc_ui_widget_button_style *style) {
    struct sc_ui_text_metrics metrics =
        sc_ui_text_measure("A", &style->text_style);
    return metrics.height + 2 * style->padding_v;
}

void
sc_ui_widget_button_init(struct sc_ui_widget_button *button, sc_ui_id id,
                         const SDL_Rect *rect, const char *label,
                         const struct sc_ui_widget_button_style *style) {
    sc_ui_button_init(&button->state, id);
    button->rect = *rect;
    button->label = label;
    button->enabled = true;
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
    if (!button->enabled) {
        struct sc_ui_button_result result = {
            .input = {false, false},
            .action = SC_UI_BUTTON_ACTION_NONE,
        };
        return result;
    }

    return sc_ui_button_handle_event(&button->state, ui, layer, &button->rect,
                                     event);
}

bool
sc_ui_widget_button_render(const struct sc_ui_widget_button *button,
                           const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_color fill = !button->enabled
                            ? button->style.fill_disabled_color
                            : button->state.pressed
                            ? button->style.fill_pressed_color
                            : button->state.hovered
                                ? button->style.fill_hover_color
                                : button->style.fill_color;
    struct sc_ui_color border = !button->enabled
                              ? button->style.border_disabled_color
                              : button->state.pressed
                              ? button->style.border_pressed_color
                              : button->style.border_color;
    struct sc_ui_text_style text_style = button->style.text_style;
    if (!button->enabled) {
        text_style.color = button->style.text_disabled_color;
    }

    SDL_Rect content_rect = sc_ui_widget_button_get_content_rect(button);

    bool ok = sc_ui_draw_fill_rect(render_ctx, &button->rect, fill);
    ok &= sc_ui_draw_rect_border(render_ctx, &button->rect, border);
    ok &= sc_ui_text_draw_in_rect(render_ctx, &content_rect, button->label,
                                  &text_style,
                                  SC_UI_TEXT_ALIGN_CENTER);
    return ok;
}
