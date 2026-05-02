#ifndef SC_UI_WIDGET_CIRCLE_BUTTON_H
#define SC_UI_WIDGET_CIRCLE_BUTTON_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#include "coords.h"
#include "ui_button.h"
#include "ui_draw.h"

struct sc_ui_widget_circle_button_style {
    struct sc_ui_color fill_color;
    struct sc_ui_color fill_hover_color;
    struct sc_ui_color fill_pressed_color;
    struct sc_ui_color fill_checked_color;
    struct sc_ui_color fill_disabled_color;
    struct sc_ui_color outline_color;
    struct sc_ui_color outline_hover_color;
    struct sc_ui_color outline_pressed_color;
    struct sc_ui_color outline_checked_color;
    struct sc_ui_color outline_disabled_color;
    struct sc_ui_color icon_color;
};

struct sc_ui_widget_circle_button_icon {
    const uint64_t *rows;
    int width;
    int height;
    int size;
};

struct sc_ui_widget_circle_button_marker {
    bool enabled;
    struct sc_point center;
    int32_t radius;
    struct sc_ui_color color;
};

struct sc_ui_widget_circle_button_outer_outline {
    bool enabled;
    int32_t radius;
    struct sc_ui_color color;
    bool dashed;
};

struct sc_ui_widget_circle_button {
    struct sc_ui_button_state state;
    struct sc_point center;
    int32_t radius;
    bool enabled;
    bool checked;
    struct sc_ui_widget_circle_button_style style;
    struct sc_ui_widget_circle_button_icon icon;
    struct sc_ui_widget_circle_button_marker marker;
    struct sc_ui_widget_circle_button_outer_outline outer_outline;
};

struct sc_ui_widget_circle_button_style
sc_ui_widget_circle_button_style_default(void);

void
sc_ui_widget_circle_button_init(struct sc_ui_widget_circle_button *button,
                                sc_ui_id id, struct sc_point center,
                                int32_t radius);

void
sc_ui_widget_circle_button_reset(struct sc_ui_widget_circle_button *button,
                                 struct sc_ui_context *ui,
                                 struct sc_ui_layer *layer);

struct sc_ui_button_result
sc_ui_widget_circle_button_handle_event(
    struct sc_ui_widget_circle_button *button, struct sc_ui_context *ui,
    struct sc_ui_layer *layer, const struct sc_ui_event *event);

bool
sc_ui_widget_circle_button_render(
    const struct sc_ui_widget_circle_button *button,
    const struct sc_ui_render_ctx *render_ctx);

#endif
