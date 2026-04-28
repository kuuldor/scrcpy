#include "ui_widget_button.h"

struct sc_ui_widget_button_style
sc_ui_widget_button_style_default(void) {
    return (struct sc_ui_widget_button_style) {
        .fill_color = sc_ui_color_rgba(0x40, 0x40, 0x50, 0xBB),
        .fill_hover_color = sc_ui_color_rgba(0x50, 0x50, 0x60, 0xBB),
        .fill_pressed_color = sc_ui_color_rgba(0x30, 0x30, 0x40, 0xBB),
        .fill_disabled_color = sc_ui_color_rgba(0x30, 0x30, 0x38, 0x90),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xB0),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .border_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x50),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 2,
            .tracking = 2,
        },
        .text_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x80),
        .padding_h = 12,
        .padding_v = 10,
    };
}

struct sc_ui_widget_button_style
sc_ui_widget_button_style_variant(enum sc_ui_widget_button_variant variant) {
    struct sc_ui_widget_button_style style = sc_ui_widget_button_style_default();
    switch (variant) {
        case SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS:
            style.fill_color = sc_ui_color_rgba(0x26, 0x7A, 0x3C, 0xBB);
            style.fill_hover_color = sc_ui_color_rgba(0x36, 0x8A, 0x4C, 0xBB);
            style.fill_pressed_color = sc_ui_color_rgba(0x16, 0x6A, 0x2C, 0xBB);
            style.fill_disabled_color = sc_ui_color_rgba(0x1A, 0x1A, 0x20, 0x90);
            break;
        case SC_UI_WIDGET_BUTTON_VARIANT_DANGER:
            style.fill_color = sc_ui_color_rgba(0x9A, 0x2A, 0x2A, 0xBB);
            style.fill_hover_color = sc_ui_color_rgba(0xAA, 0x3A, 0x3A, 0xBB);
            style.fill_pressed_color = sc_ui_color_rgba(0x8A, 0x1A, 0x1A, 0xBB);
            break;
        case SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT:
        default:
            break;
    }

    return style;
}

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
sc_ui_widget_button_init_default(struct sc_ui_widget_button *button,
                                 sc_ui_id id, const char *label) {
    SDL_Rect rect = {0, 0, 0, 0};
    struct sc_ui_widget_button_style style = sc_ui_widget_button_style_default();
    sc_ui_widget_button_init(button, id, &rect, label, &style);
}

void
sc_ui_widget_button_fit_to_content(struct sc_ui_widget_button *button) {
    button->rect.w = sc_ui_widget_button_width_for_label(button->label,
                                                         &button->style);
    button->rect.h = sc_ui_widget_button_height_for_style(&button->style);
}

void
sc_ui_widget_button_apply_variant(struct sc_ui_widget_button *button,
                                  enum sc_ui_widget_button_variant variant) {
    button->style = sc_ui_widget_button_style_variant(variant);
}

int32_t
sc_ui_widget_button_max_width(const struct sc_ui_widget_button *buttons,
                              size_t count) {
    int32_t width = 0;
    for (size_t i = 0; i < count; ++i) {
        int32_t item_width = sc_ui_widget_button_width_for_label(buttons[i].label,
                                                                 &buttons[i].style);
        if (item_width > width) {
            width = item_width;
        }
    }

    return width;
}

int32_t
sc_ui_widget_button_max_height(const struct sc_ui_widget_button *buttons,
                               size_t count) {
    int32_t height = 0;
    for (size_t i = 0; i < count; ++i) {
        int32_t item_height = sc_ui_widget_button_height_for_style(&buttons[i].style);
        if (item_height > height) {
            height = item_height;
        }
    }

    return height;
}

void
sc_ui_widget_button_set_size(struct sc_ui_widget_button *button,
                             int32_t width, int32_t height) {
    button->rect.w = width;
    button->rect.h = height;
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
