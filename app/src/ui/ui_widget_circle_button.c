#include "ui_widget_circle_button.h"

#include <SDL2/SDL_mouse.h>

struct sc_ui_widget_circle_button_style
sc_ui_widget_circle_button_style_default(void) {
    return (struct sc_ui_widget_circle_button_style) {
        .fill_color = sc_ui_color_rgba(0x40, 0x40, 0x50, 0xBB),
        .fill_hover_color = sc_ui_color_rgba(0x50, 0x50, 0x60, 0xBB),
        .fill_pressed_color = sc_ui_color_rgba(0x30, 0x30, 0x40, 0xBB),
        .fill_checked_color = sc_ui_color_rgba(0x40, 0x60, 0x90, 0xCC),
        .fill_disabled_color = sc_ui_color_rgba(0x30, 0x30, 0x38, 0x90),
        .outline_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xA0),
        .outline_hover_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xD0),
        .outline_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .outline_checked_color = sc_ui_color_rgba(0xFF, 0xD2, 0x3F, 0xFF),
        .outline_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x50),
        .icon_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xB0),
    };
}

static bool
sc_ui_widget_circle_button_hit_test(
        const struct sc_ui_widget_circle_button *button,
        int32_t x, int32_t y) {
    int32_t dx = x - button->center.x;
    int32_t dy = y - button->center.y;
    int64_t dist2 = (int64_t) dx * dx + (int64_t) dy * dy;
    int64_t radius2 = (int64_t) button->radius * button->radius;
    return dist2 <= radius2;
}

void
sc_ui_widget_circle_button_init(struct sc_ui_widget_circle_button *button,
                                sc_ui_id id, struct sc_point center,
                                int32_t radius) {
    sc_ui_button_init(&button->state, id);
    button->center = center;
    button->radius = radius;
    button->enabled = true;
    button->checked = false;
    button->style = sc_ui_widget_circle_button_style_default();
    button->icon = (struct sc_ui_widget_circle_button_icon) {
        .rows = NULL,
        .width = 0,
        .height = 0,
        .size = 24,
    };
    button->marker = (struct sc_ui_widget_circle_button_marker) {
        .enabled = false,
    };
    button->outer_outline = (struct sc_ui_widget_circle_button_outer_outline) {
        .enabled = false,
    };
}

void
sc_ui_widget_circle_button_reset(struct sc_ui_widget_circle_button *button,
                                 struct sc_ui_context *ui,
                                 struct sc_ui_layer *layer) {
    sc_ui_button_reset(&button->state, ui, layer);
}

struct sc_ui_button_result
sc_ui_widget_circle_button_handle_event(
        struct sc_ui_widget_circle_button *button, struct sc_ui_context *ui,
        struct sc_ui_layer *layer, const struct sc_ui_event *event) {
    struct sc_ui_button_result result = {
        .input = {false, false},
        .action = SC_UI_BUTTON_ACTION_NONE,
    };

    if (!button->enabled) {
        return result;
    }

    bool captured = sc_ui_context_get_active_id(ui) == button->state.id
                 && sc_ui_context_get_capture_layer(ui) == layer;

    switch (event->type) {
        case SC_UI_EVENT_POINTER_MOVE: {
            bool hovered = sc_ui_widget_circle_button_hit_test(
                button, event->data.pointer.x, event->data.pointer.y);
            if (hovered != button->state.hovered) {
                button->state.hovered = hovered;
                result.input.request_refresh = true;
            }

            if (hovered) {
                sc_ui_context_set_hover_id(ui, button->state.id);
            } else if (sc_ui_context_get_hover_id(ui) == button->state.id) {
                sc_ui_context_set_hover_id(ui, SC_UI_ID_INVALID);
                result.input.request_refresh = true;
            }

            result.input.consumed = hovered || captured;
            break;
        }
        case SC_UI_EVENT_POINTER_DOWN: {
            bool inside = sc_ui_widget_circle_button_hit_test(
                button, event->data.pointer.x, event->data.pointer.y);
            if (inside && event->data.pointer.button == SDL_BUTTON_LEFT) {
                button->state.hovered = true;
                button->state.pressed = true;
                sc_ui_context_set_hover_id(ui, button->state.id);
                sc_ui_context_set_active_id(ui, button->state.id);
                sc_ui_context_capture_pointer(ui, layer);
                result.input.consumed = true;
                result.input.request_refresh = true;
                result.action = SC_UI_BUTTON_ACTION_PRESS;
            }
            break;
        }
        case SC_UI_EVENT_POINTER_UP: {
            bool inside = sc_ui_widget_circle_button_hit_test(
                button, event->data.pointer.x, event->data.pointer.y);
            bool pressed = button->state.pressed || captured;
            if (pressed && event->data.pointer.button == SDL_BUTTON_LEFT) {
                button->state.pressed = false;
                button->state.hovered = inside;
                if (inside) {
                    sc_ui_context_set_hover_id(ui, button->state.id);
                } else if (sc_ui_context_get_hover_id(ui) == button->state.id) {
                    sc_ui_context_set_hover_id(ui, SC_UI_ID_INVALID);
                }
                if (sc_ui_context_get_active_id(ui) == button->state.id) {
                    sc_ui_context_set_active_id(ui, SC_UI_ID_INVALID);
                }
                if (sc_ui_context_get_capture_layer(ui) == layer) {
                    sc_ui_context_release_pointer_capture(ui);
                }
                result.input.consumed = true;
                result.input.request_refresh = true;
                result.action = inside ? SC_UI_BUTTON_ACTION_CLICK
                                       : SC_UI_BUTTON_ACTION_RELEASE;
            }
            break;
        }
        case SC_UI_EVENT_CANCEL:
        case SC_UI_EVENT_FOCUS_LOST:
            if (button->state.hovered || button->state.pressed || captured) {
                sc_ui_widget_circle_button_reset(button, ui, layer);
                result.input.request_refresh = true;
            }
            break;
        default:
            break;
    }

    return result;
}

bool
sc_ui_widget_circle_button_render(
        const struct sc_ui_widget_circle_button *button,
        const struct sc_ui_render_ctx *render_ctx) {
    bool active = render_ctx->ui
               && sc_ui_context_get_active_id(render_ctx->ui)
                  == button->state.id;
    bool hovered = button->state.hovered
                || (render_ctx->ui
                    && sc_ui_context_get_hover_id(render_ctx->ui)
                       == button->state.id);
    bool pressed = button->state.pressed || active;
    struct sc_ui_color fill = !button->enabled
                            ? button->style.fill_disabled_color
                            : pressed
                            ? button->style.fill_pressed_color
                            : hovered
                            ? button->style.fill_hover_color
                            : button->checked
                            ? button->style.fill_checked_color
                            : button->style.fill_color;
    struct sc_ui_color outline = !button->enabled
                               ? button->style.outline_disabled_color
                               : pressed
                               ? button->style.outline_pressed_color
                               : hovered
                               ? button->style.outline_hover_color
                               : button->checked
                               ? button->style.outline_checked_color
                               : button->style.outline_color;

    bool ok = sc_ui_draw_fill_circle(render_ctx, button->center,
                                     button->radius, fill);
    ok &= sc_ui_draw_circle_outline(render_ctx, button->center,
                                    button->radius, outline, false);

    if (button->outer_outline.enabled) {
        ok &= sc_ui_draw_circle_outline(render_ctx, button->center,
                                        button->outer_outline.radius,
                                        button->outer_outline.color,
                                        button->outer_outline.dashed);
    }

    if (button->marker.enabled) {
        ok &= sc_ui_draw_fill_circle(render_ctx, button->marker.center,
                                     button->marker.radius,
                                     button->marker.color);
    }

    if (button->icon.rows && button->icon.width > 0
            && button->icon.height > 0) {
        int32_t size = button->icon.size > 0 ? button->icon.size : 24;
        int32_t width = size;
        int32_t height = size;
        if (button->icon.width > button->icon.height) {
            height = (int64_t) size * button->icon.height / button->icon.width;
            if (height < 1) {
                height = 1;
            }
        } else if (button->icon.height > button->icon.width) {
            width = (int64_t) size * button->icon.width / button->icon.height;
            if (width < 1) {
                width = 1;
            }
        }

        SDL_Rect rect = {
            .x = button->center.x - width / 2,
            .y = button->center.y - height / 2,
            .w = width,
            .h = height,
        };
        ok &= sc_ui_draw_bitmap_icon_in_rect(render_ctx, &rect,
                                             button->icon.rows,
                                             button->icon.width,
                                             button->icon.height,
                                             button->style.icon_color);
    }

    return ok;
}
