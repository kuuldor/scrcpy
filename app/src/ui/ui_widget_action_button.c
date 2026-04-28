#include "ui_widget_action_button.h"

void
sc_ui_widget_action_button_init(struct sc_ui_widget_action_button *action_button,
                                sc_ui_id id, const char *label,
                                sc_ui_widget_action_handler action,
                                void *userdata) {
    sc_ui_widget_button_init_default(&action_button->button, id, label);
    action_button->action = action;
    action_button->userdata = userdata;
    action_button->margin = 8;
}

void
sc_ui_widget_action_button_layout_top_right(
    struct sc_ui_widget_action_button *action_button, struct sc_size bounds) {
    sc_ui_widget_button_fit_to_content(&action_button->button);
    sc_ui_widget_button_place_top_right(&action_button->button, bounds,
                                        action_button->margin);
}

void
sc_ui_widget_action_button_apply_variant(
    struct sc_ui_widget_action_button *action_button,
    enum sc_ui_widget_button_variant variant) {
    sc_ui_widget_button_apply_variant(&action_button->button, variant);
}

void
sc_ui_widget_action_button_set_label_and_layout_top_right(
    struct sc_ui_widget_action_button *action_button,
    const char *label, struct sc_size bounds) {
    sc_ui_widget_button_set_label(&action_button->button, label);
    sc_ui_widget_action_button_layout_top_right(action_button, bounds);
}

struct sc_ui_input_result
sc_ui_widget_action_button_handle_event(
    struct sc_ui_widget_action_button *action_button,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event) {
    struct sc_ui_button_result button_result =
        sc_ui_widget_button_handle_event(&action_button->button, ui, layer,
                                         event);
    struct sc_ui_input_result result = button_result.input;
    if (button_result.action == SC_UI_BUTTON_ACTION_CLICK
            && action_button->action) {
        action_button->action(action_button->userdata, &result);
    }
    return result;
}

bool
sc_ui_widget_action_button_render(
    const struct sc_ui_widget_action_button *action_button,
    const struct sc_ui_render_ctx *render_ctx) {
    return sc_ui_widget_button_render(&action_button->button, render_ctx);
}
