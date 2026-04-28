#ifndef SC_UI_WIDGET_ACTION_BUTTON_H
#define SC_UI_WIDGET_ACTION_BUTTON_H

#include "common.h"

#include "coords.h"
#include "ui_widget_button.h"

#define SC_UI_WIDGET_ACTION_BUTTON_ACTION_NONE (-1)

struct sc_ui_widget_action_button_result {
    struct sc_ui_input_result input;
    int action_id;
};

struct sc_ui_widget_action_button {
    struct sc_ui_widget_button button;
    int action_id;
    int margin;
};

void
sc_ui_widget_action_button_init(struct sc_ui_widget_action_button *action_button,
                                sc_ui_id id, const char *label, int action_id);

void
sc_ui_widget_action_button_layout_top_right(
    struct sc_ui_widget_action_button *action_button, struct sc_size bounds);

struct sc_ui_widget_action_button_result
sc_ui_widget_action_button_handle_event(
    struct sc_ui_widget_action_button *action_button,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event);

bool
sc_ui_widget_action_button_render(
    const struct sc_ui_widget_action_button *action_button,
    const struct sc_ui_render_ctx *render_ctx);

#endif
