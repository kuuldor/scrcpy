#include "ui_touchmap_layer.h"

#include <SDL2/SDL.h>

#include "ui/ui_context.h"
#include "ui/ui_geom.h"
#include "ui/ui_id.h"
#include "touchmap/touchmap_overlay.h"
#include "touchmap/touchmap_state.h"

#define SC_UI_TOUCHMAP_EDIT_MARGIN 8
#define SC_UI_TOUCHMAP_TOOLBAR_PADDING 6
#define SC_UI_TOUCHMAP_TOOLBAR_GAP 6
#define SC_UI_TOUCHMAP_MENU_GAP 6

static const char *
sc_ui_touchmap_layer_get_edit_label(const struct sc_touchmap_state *state) {
    return !state->map ? "NEW"
         : state->edit_mode ? "QUIT"
         : "EDIT";
}

static void
sc_ui_touchmap_layer_action_edit(void *userdata,
                                 struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = !tm->touchmap_state->map
        ? SC_TOUCHMAP_OVERLAY_CONTROL_NEW
        : SC_TOUCHMAP_OVERLAY_CONTROL_EDIT;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_menu_toggle(void *userdata,
                                        struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->add_menu_open = !tm->touchmap_state->add_menu_open;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_delete(void *userdata,
                                   struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_DEL;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_quit(void *userdata,
                                 struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_QUIT;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_button(void *userdata,
                                       struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON;
    tm->touchmap_state->add_menu_open = false;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_skill(void *userdata,
                                      struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL;
    tm->touchmap_state->add_menu_open = false;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_action_add_walk(void *userdata,
                                     struct sc_ui_input_result *result) {
    struct sc_ui_touchmap_layer *tm = userdata;
    tm->touchmap_state->pending_control = SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK;
    tm->touchmap_state->add_menu_open = false;
    result->request_refresh = true;
}

static void
sc_ui_touchmap_layer_sync_layout(struct sc_ui_touchmap_layer *tm,
                                 const struct sc_ui_geometry *geometry) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);
    static const enum sc_ui_widget_button_variant toolbar_variants[] = {
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DANGER,
    };
    static const enum sc_ui_widget_button_variant menu_variants[] = {
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
        SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT,
    };

    const char *edit_label = sc_ui_touchmap_layer_get_edit_label(state);
    sc_ui_widget_action_button_apply_variant(&tm->edit_button,
                                             SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    tm->edit_button.margin = SC_UI_TOUCHMAP_EDIT_MARGIN;
    sc_ui_widget_action_button_set_label_and_layout_top_right(&tm->edit_button,
                                                              edit_label,
                                                              logical_size);

    sc_ui_widget_action_menu_apply_variants(&tm->action_menu,
                                            toolbar_variants,
                                            menu_variants);

    sc_ui_widget_action_menu_set_spacing(&tm->action_menu,
                                         SC_UI_TOUCHMAP_EDIT_MARGIN,
                                         SC_UI_TOUCHMAP_TOOLBAR_PADDING,
                                         SC_UI_TOUCHMAP_TOOLBAR_GAP,
                                         SC_UI_TOUCHMAP_MENU_GAP);
    sc_ui_widget_action_menu_layout_top_right(&tm->action_menu, logical_size);
    sc_ui_widget_action_menu_set_menu_button_enabled(
        &tm->action_menu, 2, !(state->map && state->map->has_walk));
}

static bool
sc_ui_touchmap_layer_is_visible(const struct sc_ui_touchmap_layer *layer) {
    return layer->touchmap_state
        && (layer->touchmap_state->overlay_enabled
            || layer->touchmap_state->edit_mode);
}

static void
sc_ui_touchmap_layer_sync(struct sc_ui_layer *layer, struct sc_ui_context *ui) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);
    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return;
    }

    sc_ui_touchmap_layer_sync_layout(tm, geometry);
}

static struct sc_ui_input_result
sc_ui_touchmap_layer_handle_event(struct sc_ui_layer *layer,
                                  struct sc_ui_context *ui,
                                  const struct sc_ui_event *event) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_touchmap_state *state = tm->touchmap_state;

    struct sc_ui_input_result result = {false, false};

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return result;
    }

    if (state->edit_mode) {
        struct sc_ui_widget_action_menu_result menu_result =
            sc_ui_widget_action_menu_handle_event(&tm->action_menu, ui, layer,
                                                  event,
                                                  tm->touchmap_state->add_menu_open);
        result = menu_result.input;
        if (!result.consumed && menu_result.clicked_outside) {
            tm->touchmap_state->add_menu_open = false;
            result.request_refresh = true;
        }
    } else {
        result = sc_ui_widget_action_button_handle_event(&tm->edit_button, ui,
                                                         layer, event);
    }

    return result;
}

static bool
sc_ui_touchmap_layer_render(struct sc_ui_layer *layer,
                            const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_touchmap_state *state = tm->touchmap_state;

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return true;
    }

    bool ok = true;

    if (state->edit_mode) {
        ok &= sc_ui_widget_action_menu_render(&tm->action_menu, render_ctx,
                                              state->add_menu_open);
    } else {
        ok &= sc_ui_widget_action_button_render(&tm->edit_button, render_ctx);
    }

    return ok;
}

void
sc_ui_touchmap_layer_init(struct sc_ui_touchmap_layer *layer,
                          struct sc_touchmap_state *touchmap_state) {
    static const struct sc_ui_layer_ops ops = {
        .sync = sc_ui_touchmap_layer_sync,
        .handle_event = sc_ui_touchmap_layer_handle_event,
        .render = sc_ui_touchmap_layer_render,
        .on_detach = NULL,
    };

    layer->touchmap_state = touchmap_state;
    layer->layer = (struct sc_ui_layer) {
        .ops = &ops,
        .visible = true,
        .enabled = true,
        .z_index = 10,
        .userdata = layer,
    };

    sc_ui_widget_action_button_init(&layer->edit_button,
                                    sc_ui_id_from_u32(layer, 0), "EDIT",
                                    sc_ui_touchmap_layer_action_edit, layer);

    sc_ui_widget_button_init_default(&layer->toolbar_buttons[0],
                                     sc_ui_id_from_u32(layer, 1), "ADD");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[1],
                                     sc_ui_id_from_u32(layer, 2), "DEL");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[2],
                                     sc_ui_id_from_u32(layer, 3), "QUIT");

    sc_ui_widget_button_init_default(&layer->add_menu_items[0],
                                     sc_ui_id_from_u32(layer, 4), "BUTTON");
    sc_ui_widget_button_init_default(&layer->add_menu_items[1],
                                     sc_ui_id_from_u32(layer, 5), "SKILL");
    sc_ui_widget_button_init_default(&layer->add_menu_items[2],
                                     sc_ui_id_from_u32(layer, 6), "WALK");

    struct sc_ui_widget_button *toolbar_buttons[] = {
        &layer->toolbar_buttons[0],
        &layer->toolbar_buttons[1],
        &layer->toolbar_buttons[2],
    };
    static const sc_ui_widget_action_handler toolbar_actions[] = {
        sc_ui_touchmap_layer_action_menu_toggle,
        sc_ui_touchmap_layer_action_delete,
        sc_ui_touchmap_layer_action_quit,
    };
    struct sc_ui_widget_button *menu_buttons[] = {
        &layer->add_menu_items[0],
        &layer->add_menu_items[1],
        &layer->add_menu_items[2],
    };
    static const sc_ui_widget_action_handler menu_actions[] = {
        sc_ui_touchmap_layer_action_add_button,
        sc_ui_touchmap_layer_action_add_skill,
        sc_ui_touchmap_layer_action_add_walk,
    };
    sc_ui_widget_action_menu_init(&layer->action_menu, toolbar_buttons,
                                  ARRAY_LEN(toolbar_buttons), toolbar_actions,
                                  menu_buttons, ARRAY_LEN(menu_buttons),
                                  menu_actions, layer);
}
