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

enum sc_ui_touchmap_action_id {
    SC_UI_TOUCHMAP_ACTION_NONE = SC_UI_WIDGET_ACTION_MENU_ACTION_NONE,
    SC_UI_TOUCHMAP_ACTION_EDIT = SC_UI_WIDGET_ACTION_BUTTON_ACTION_NONE + 1,
    SC_UI_TOUCHMAP_ACTION_MENU_TOGGLE = 1,
    SC_UI_TOUCHMAP_ACTION_DELETE,
    SC_UI_TOUCHMAP_ACTION_QUIT,
    SC_UI_TOUCHMAP_ACTION_ADD_BUTTON,
    SC_UI_TOUCHMAP_ACTION_ADD_SKILL,
    SC_UI_TOUCHMAP_ACTION_ADD_WALK,
};

static void
sc_ui_touchmap_layer_sync_layout(struct sc_ui_touchmap_layer *tm,
                                 const struct sc_ui_geometry *geometry) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);

    const char *edit_label = !state->map ? "NEW"
                            : state->edit_mode ? "QUIT"
                            : "EDIT";
    sc_ui_widget_button_apply_variant(&tm->edit_button.button,
                                      SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    sc_ui_widget_button_set_label(&tm->edit_button.button, edit_label);
    tm->edit_button.margin = SC_UI_TOUCHMAP_EDIT_MARGIN;
    sc_ui_widget_action_button_layout_top_right(&tm->edit_button, logical_size);

    sc_ui_widget_button_apply_variant(&tm->toolbar_buttons[0],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    sc_ui_widget_button_apply_variant(&tm->toolbar_buttons[1],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    sc_ui_widget_button_apply_variant(&tm->toolbar_buttons[2],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DANGER);
    sc_ui_widget_button_apply_variant(&tm->add_menu_items[0],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    sc_ui_widget_button_apply_variant(&tm->add_menu_items[1],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    sc_ui_widget_button_apply_variant(&tm->add_menu_items[2],
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);

    struct sc_ui_widget_button *sized_buttons[] = {
        &tm->toolbar_buttons[0],
        &tm->toolbar_buttons[1],
        &tm->toolbar_buttons[2],
        &tm->add_menu_items[0],
        &tm->add_menu_items[1],
        &tm->add_menu_items[2],
    };
    int32_t button_width = 0;
    int32_t button_height = 0;
    for (size_t i = 0; i < ARRAY_LEN(sized_buttons); ++i) {
        sc_ui_widget_button_fit_to_content(sized_buttons[i]);
        if (sized_buttons[i]->rect.w > button_width) {
            button_width = sized_buttons[i]->rect.w;
        }
        if (sized_buttons[i]->rect.h > button_height) {
            button_height = sized_buttons[i]->rect.h;
        }
    }

    for (int i = 0; i < 3; ++i) {
        sc_ui_widget_button_set_size(&tm->toolbar_buttons[i], button_width,
                                     button_height);
    }

    for (size_t i = 0; i < 3; ++i) {
        sc_ui_widget_button_set_size(&tm->add_menu_items[i], button_width,
                                     button_height);
    }

    tm->action_menu.margin = SC_UI_TOUCHMAP_EDIT_MARGIN;
    tm->action_menu.padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    tm->action_menu.gap = SC_UI_TOUCHMAP_TOOLBAR_GAP;
    tm->action_menu.menu_gap = SC_UI_TOUCHMAP_MENU_GAP;
    sc_ui_widget_action_menu_layout_top_right(&tm->action_menu, logical_size);
    tm->add_menu_items[2].enabled = !(state->map && state->map->has_walk);
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
        switch (menu_result.action_id) {
            case SC_UI_TOUCHMAP_ACTION_MENU_TOGGLE:
                tm->touchmap_state->add_menu_open =
                    !tm->touchmap_state->add_menu_open;
                result.request_refresh = true;
                break;
            case SC_UI_TOUCHMAP_ACTION_DELETE:
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_DEL;
                result.request_refresh = true;
                break;
            case SC_UI_TOUCHMAP_ACTION_QUIT:
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_QUIT;
                result.request_refresh = true;
                break;
            case SC_UI_TOUCHMAP_ACTION_ADD_BUTTON:
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON;
                tm->touchmap_state->add_menu_open = false;
                result.request_refresh = true;
                break;
            case SC_UI_TOUCHMAP_ACTION_ADD_SKILL:
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL;
                tm->touchmap_state->add_menu_open = false;
                result.request_refresh = true;
                break;
            case SC_UI_TOUCHMAP_ACTION_ADD_WALK:
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK;
                tm->touchmap_state->add_menu_open = false;
                result.request_refresh = true;
                break;
            default:
                if (!result.consumed && menu_result.clicked_outside
                        && !sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                     event->data.pointer.y,
                                                     &tm->edit_button.button.rect)) {
                    tm->touchmap_state->add_menu_open = false;
                    result.request_refresh = true;
                }
                break;
        }
    } else {
        struct sc_ui_widget_action_button_result edit_result =
            sc_ui_widget_action_button_handle_event(&tm->edit_button, ui,
                                                    layer, event);
        result = edit_result.input;
        if (edit_result.action_id == SC_UI_TOUCHMAP_ACTION_EDIT) {
            if (!tm->touchmap_state->map) {
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_NEW;
            } else {
                tm->touchmap_state->pending_control =
                    SC_TOUCHMAP_OVERLAY_CONTROL_EDIT;
            }
            result.request_refresh = true;
        }
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
                                    SC_UI_TOUCHMAP_ACTION_EDIT);

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
    static const int toolbar_action_ids[] = {
        SC_UI_TOUCHMAP_ACTION_MENU_TOGGLE,
        SC_UI_TOUCHMAP_ACTION_DELETE,
        SC_UI_TOUCHMAP_ACTION_QUIT,
    };
    struct sc_ui_widget_button *menu_buttons[] = {
        &layer->add_menu_items[0],
        &layer->add_menu_items[1],
        &layer->add_menu_items[2],
    };
    static const int menu_action_ids[] = {
        SC_UI_TOUCHMAP_ACTION_ADD_BUTTON,
        SC_UI_TOUCHMAP_ACTION_ADD_SKILL,
        SC_UI_TOUCHMAP_ACTION_ADD_WALK,
    };
    sc_ui_widget_action_menu_init(&layer->action_menu, toolbar_buttons,
                                  ARRAY_LEN(toolbar_buttons), toolbar_action_ids,
                                  menu_buttons, ARRAY_LEN(menu_buttons),
                                  menu_action_ids);
}
