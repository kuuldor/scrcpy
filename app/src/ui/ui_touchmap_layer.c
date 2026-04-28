#include "ui_touchmap_layer.h"

#include <SDL2/SDL.h>

#include "ui_context.h"
#include "ui_geom.h"
#include "ui_id.h"
#include "ui_widget_panel.h"
#include "touchmap_overlay.h"
#include "touchmap_state.h"

#define SC_UI_TOUCHMAP_EDIT_MARGIN 8
#define SC_UI_TOUCHMAP_TOOLBAR_PADDING 6
#define SC_UI_TOUCHMAP_TOOLBAR_GAP 6
#define SC_UI_TOUCHMAP_MENU_GAP 6

static void
sc_ui_touchmap_layer_sync_layout(struct sc_ui_touchmap_layer *tm,
                                 const struct sc_ui_geometry *geometry) {
    const struct sc_touchmap_state *state = tm->touchmap_state;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);

    const char *edit_label = !state->map ? "NEW"
                            : state->edit_mode ? "QUIT"
                            : "EDIT";
    tm->edit_button.label = edit_label;
    sc_ui_widget_button_apply_variant(&tm->edit_button,
                                      SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    sc_ui_widget_button_fit_to_content(&tm->edit_button);
    tm->edit_button.rect.x = logical_size.width - tm->edit_button.rect.w
                           - SC_UI_TOUCHMAP_EDIT_MARGIN;
    tm->edit_button.rect.y = SC_UI_TOUCHMAP_EDIT_MARGIN;

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

    tm->toolbar_panel.style.padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    tm->toolbar_panel.rect.w = 3 * button_width + 2 * SC_UI_TOUCHMAP_TOOLBAR_GAP
                             + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    tm->toolbar_panel.rect.h = button_height + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    sc_ui_widget_panel_place_top_right(&tm->toolbar_panel, logical_size,
                                       SC_UI_TOUCHMAP_EDIT_MARGIN);

    for (int i = 0; i < 3; ++i) {
        sc_ui_widget_button_set_size(&tm->toolbar_buttons[i], button_width,
                                     button_height);
        tm->toolbar_buttons[i].rect.x = tm->toolbar_panel.rect.x
            + SC_UI_TOUCHMAP_TOOLBAR_PADDING
            + i * (button_width + SC_UI_TOUCHMAP_TOOLBAR_GAP);
        tm->toolbar_buttons[i].rect.y = tm->toolbar_panel.rect.y
                                      + SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    }

    tm->add_menu.panel.style.padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    tm->add_menu.style.panel.padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    tm->add_menu.style.item_width = button_width;
    tm->add_menu.style.item_height = button_height;
    tm->add_menu.style.item_gap = SC_UI_TOUCHMAP_MENU_GAP;
    sc_ui_widget_menu_fit_panel(&tm->add_menu, 3);
    sc_ui_widget_menu_place_below(&tm->add_menu, &tm->toolbar_panel.rect,
                                  SC_UI_TOUCHMAP_MENU_GAP);

    for (size_t i = 0; i < 3; ++i) {
        sc_ui_widget_button_set_size(&tm->add_menu_items[i], button_width,
                                     button_height);
        tm->add_menu_items[i].rect = sc_ui_widget_menu_get_item_rect(&tm->add_menu,
                                                                     i);
    }
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
        for (int i = 0; i < 3; ++i) {
            struct sc_ui_button_result btn_result =
                sc_ui_widget_button_handle_event(&tm->toolbar_buttons[i],
                                                 ui, layer, event);
            result = btn_result.input;
            if (btn_result.action == SC_UI_BUTTON_ACTION_CLICK) {
                if (i == 0) {
                    tm->touchmap_state->add_menu_open =
                        !tm->touchmap_state->add_menu_open;
                    result.request_refresh = true;
                } else if (i == 1) {
                    tm->touchmap_state->pending_control =
                        SC_TOUCHMAP_OVERLAY_CONTROL_DEL;
                    result.request_refresh = true;
                } else if (i == 2) {
                    tm->touchmap_state->pending_control =
                        SC_TOUCHMAP_OVERLAY_CONTROL_QUIT;
                    result.request_refresh = true;
                }
                break;
            }
            if (result.consumed) {
                break;
            }
        }

        if (!result.consumed && tm->touchmap_state->add_menu_open) {
            for (size_t i = 0; i < 3; ++i) {
                tm->add_menu_items[i].rect =
                    sc_ui_widget_menu_get_item_rect(&tm->add_menu, i);
            }

            for (size_t i = 0; i < 3; ++i) {
                if (!tm->add_menu_items[i].enabled) {
                    continue;
                }
                struct sc_ui_button_result item_result =
                    sc_ui_widget_button_handle_event(&tm->add_menu_items[i],
                                                     ui, layer, event);
                result = item_result.input;
                if (item_result.action == SC_UI_BUTTON_ACTION_CLICK) {
                    if (i == 0) {
                        tm->touchmap_state->pending_control =
                            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_BUTTON;
                    } else if (i == 1) {
                        tm->touchmap_state->pending_control =
                            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_SKILL;
                    } else if (i == 2) {
                        tm->touchmap_state->pending_control =
                            SC_TOUCHMAP_OVERLAY_CONTROL_ADD_WALK;
                    }
                    tm->touchmap_state->add_menu_open = false;
                    result.request_refresh = true;
                    break;
                }
            }

            if (!result.consumed
                    && event->type == SC_UI_EVENT_POINTER_DOWN) {
                bool in_menu = sc_ui_geom_point_in_rect(
                    event->data.pointer.x, event->data.pointer.y,
                    &tm->add_menu.panel.rect);
                if (!in_menu) {
                    bool in_toolbar = sc_ui_geom_point_in_rect(
                        event->data.pointer.x, event->data.pointer.y,
                        &tm->toolbar_panel.rect);
                    bool in_edit = sc_ui_geom_point_in_rect(
                        event->data.pointer.x, event->data.pointer.y,
                        &tm->edit_button.rect);
                    if (!in_toolbar && !in_edit) {
                        tm->touchmap_state->add_menu_open = false;
                        result.request_refresh = true;
                    }
                }
            }
        }
    } else {
        struct sc_ui_button_result edit_result =
            sc_ui_widget_button_handle_event(&tm->edit_button, ui, layer,
                                             event);
        result = edit_result.input;
        if (edit_result.action == SC_UI_BUTTON_ACTION_CLICK) {
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
        ok &= sc_ui_widget_panel_render(&tm->toolbar_panel, render_ctx);

        for (int i = 0; i < 3; ++i) {
            ok &= sc_ui_widget_button_render(&tm->toolbar_buttons[i],
                                              render_ctx);
        }

        if (state->add_menu_open) {
            ok &= sc_ui_widget_menu_render(&tm->add_menu, render_ctx);

            for (size_t i = 0; i < 3; ++i) {
                tm->add_menu_items[i].rect =
                    sc_ui_widget_menu_get_item_rect(&tm->add_menu, i);
                ok &= sc_ui_widget_button_render(&tm->add_menu_items[i],
                                                  render_ctx);
            }
        }
    } else {
        ok &= sc_ui_widget_button_render(&tm->edit_button, render_ctx);
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

    sc_ui_widget_panel_init_default(&layer->toolbar_panel);
    layer->toolbar_panel.style.padding = 0;

    sc_ui_widget_button_init_default(&layer->edit_button,
                                     sc_ui_id_from_u32(layer, 0), "EDIT");

    sc_ui_widget_button_init_default(&layer->toolbar_buttons[0],
                                     sc_ui_id_from_u32(layer, 1), "ADD");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[1],
                                     sc_ui_id_from_u32(layer, 2), "DEL");
    sc_ui_widget_button_init_default(&layer->toolbar_buttons[2],
                                     sc_ui_id_from_u32(layer, 3), "QUIT");

    sc_ui_widget_menu_init_default(&layer->add_menu);
    layer->add_menu.style.panel.padding = 0;
    layer->add_menu.style.item_gap = 0;

    sc_ui_widget_button_init_default(&layer->add_menu_items[0],
                                     sc_ui_id_from_u32(layer, 4), "BUTTON");
    sc_ui_widget_button_init_default(&layer->add_menu_items[1],
                                     sc_ui_id_from_u32(layer, 5), "SKILL");
    sc_ui_widget_button_init_default(&layer->add_menu_items[2],
                                     sc_ui_id_from_u32(layer, 6), "WALK");
}
