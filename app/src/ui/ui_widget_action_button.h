#ifndef SC_UI_WIDGET_ACTION_BUTTON_H
#define SC_UI_WIDGET_ACTION_BUTTON_H

#include "common.h"

#include "coords.h"
#include "ui_widget_button.h"

typedef void (*sc_ui_widget_action_handler)(void *userdata,
                                            struct sc_ui_input_result *result);

struct sc_ui_widget_action_button {
    struct sc_ui_widget_button button;
    sc_ui_widget_action_handler action;
    void *userdata;
    int margin;
};

void
sc_ui_widget_action_button_init(struct sc_ui_widget_action_button *action_button,
                                sc_ui_id id, const char *label,
                                sc_ui_widget_action_handler action,
                                void *userdata);

void
sc_ui_widget_action_button_layout_top_right(
    struct sc_ui_widget_action_button *action_button, struct sc_size bounds);

void
sc_ui_widget_action_button_apply_variant(
    struct sc_ui_widget_action_button *action_button,
    enum sc_ui_widget_button_variant variant);

void
sc_ui_widget_action_button_set_label_and_layout_top_right(
    struct sc_ui_widget_action_button *action_button,
    const char *label, struct sc_size bounds);

struct sc_ui_input_result
sc_ui_widget_action_button_handle_event(
    struct sc_ui_widget_action_button *action_button,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event);

bool
sc_ui_widget_action_button_render(
    const struct sc_ui_widget_action_button *action_button,
    const struct sc_ui_render_ctx *render_ctx);

#endif
