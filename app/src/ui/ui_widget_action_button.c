#include "ui_widget_action_button.h"

void
sc_ui_widget_action_button_init(struct sc_ui_widget_action_button *action_button,
                                sc_ui_id id, const char *label, int action_id) {
    sc_ui_widget_button_init_default(&action_button->button, id, label);
    action_button->action_id = action_id;
    action_button->margin = 8;
}

void
sc_ui_widget_action_button_layout_top_right(
    struct sc_ui_widget_action_button *action_button, struct sc_size bounds) {
    sc_ui_widget_button_fit_to_content(&action_button->button);
    sc_ui_widget_button_place_top_right(&action_button->button, bounds,
                                        action_button->margin);
}

struct sc_ui_widget_action_button_result
sc_ui_widget_action_button_handle_event(
    struct sc_ui_widget_action_button *action_button,
    struct sc_ui_context *ui, struct sc_ui_layer *layer,
    const struct sc_ui_event *event) {
    struct sc_ui_button_result button_result =
        sc_ui_widget_button_handle_event(&action_button->button, ui, layer,
                                         event);
    return (struct sc_ui_widget_action_button_result) {
        .input = button_result.input,
        .action_id = button_result.action == SC_UI_BUTTON_ACTION_CLICK
                   ? action_button->action_id
                   : SC_UI_WIDGET_ACTION_BUTTON_ACTION_NONE,
    };
}

bool
sc_ui_widget_action_button_render(
    const struct sc_ui_widget_action_button *action_button,
    const struct sc_ui_render_ctx *render_ctx) {
    return sc_ui_widget_button_render(&action_button->button, render_ctx);
}
