#include "ui_demo_layer.h"

#include <SDL2/SDL.h>

#include "ui_context.h"
#include "ui_geom.h"
#include "ui_id.h"
#include "util/log.h"

#define SC_UI_DEMO_MARGIN 12
#define SC_UI_DEMO_MENU_GAP 6

static const char *
sc_ui_demo_layer_get_primary_label(const struct sc_ui_demo_layer *demo) {
    return demo->primary_toggled ? "ON" : "DEMO";
}

static const char *
sc_ui_demo_layer_get_secondary_label(const struct sc_ui_demo_layer *demo) {
    return demo->secondary_toggled ? "OPEN" : "PING";
}

static void
sc_ui_demo_layer_action_primary(void *userdata,
                                struct sc_ui_input_result *result) {
    struct sc_ui_demo_layer *demo = userdata;
    demo->primary_toggled = !demo->primary_toggled;
    LOGI("UI demo primary button clicked (%s)",
         demo->primary_toggled ? "toggled on" : "toggled off");
    result->request_refresh = true;
}

static void
sc_ui_demo_layer_action_secondary(void *userdata,
                                  struct sc_ui_input_result *result) {
    struct sc_ui_demo_layer *demo = userdata;
    demo->secondary_toggled = !demo->secondary_toggled;
    demo->menu_open = !demo->menu_open;
    LOGI("UI demo secondary button clicked (%s, menu %s)",
         demo->secondary_toggled ? "toggled on" : "toggled off",
         demo->menu_open ? "opened" : "closed");
    result->request_refresh = true;
}

static void
sc_ui_demo_layer_action_first(void *userdata,
                              struct sc_ui_input_result *result) {
    struct sc_ui_demo_layer *demo = userdata;
    LOGI("UI demo menu item clicked (FIRST)");
    demo->menu_open = false;
    result->request_refresh = true;
}

static void
sc_ui_demo_layer_action_second(void *userdata,
                               struct sc_ui_input_result *result) {
    struct sc_ui_demo_layer *demo = userdata;
    LOGI("UI demo menu item clicked (SECOND)");
    demo->menu_open = false;
    result->request_refresh = true;
}

static bool
sc_ui_demo_layer_has_rect(const struct sc_ui_geometry *geometry,
                          const struct sc_ui_widget_action_button *primary,
                          const struct sc_ui_widget_action_menu *action_menu) {
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);
    int32_t width = primary->button.rect.w + SC_UI_DEMO_MARGIN
                  + action_menu->toolbar_panel.rect.w;
    int32_t height = primary->button.rect.h;
    if (action_menu->toolbar_panel.rect.h > height) {
        height = action_menu->toolbar_panel.rect.h;
    }

    return geometry->has_frame
        && logical_size.width >= width + 2 * SC_UI_DEMO_MARGIN
        && logical_size.height >= height + 2 * SC_UI_DEMO_MARGIN;
}

static void
sc_ui_demo_layer_sync(struct sc_ui_layer *layer, struct sc_ui_context *ui) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);
    struct sc_size logical_size = sc_ui_geom_get_logical_size(geometry);

    sc_ui_widget_action_button_apply_variant(&demo->primary_button,
                                             SC_UI_WIDGET_BUTTON_VARIANT_SUCCESS);
    demo->primary_button.margin = SC_UI_DEMO_MARGIN;
    sc_ui_widget_action_button_set_label_and_layout_top_right(
        &demo->primary_button, sc_ui_demo_layer_get_primary_label(demo),
        (struct sc_size) {
            .width = logical_size.width - demo->action_menu.toolbar_panel.rect.w
                   - SC_UI_DEMO_MARGIN,
            .height = logical_size.height,
        });

    sc_ui_widget_button_apply_variant(&demo->secondary_button,
                                      SC_UI_WIDGET_BUTTON_VARIANT_DANGER);
    sc_ui_widget_button_set_label(&demo->secondary_button,
                                  sc_ui_demo_layer_get_secondary_label(demo));
    sc_ui_widget_button_apply_variant(&demo->menu_item_one,
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    sc_ui_widget_button_apply_variant(&demo->menu_item_two,
                                      SC_UI_WIDGET_BUTTON_VARIANT_DEFAULT);
    demo->menu_item_two.enabled = false;

    sc_ui_widget_action_menu_set_spacing(&demo->action_menu, SC_UI_DEMO_MARGIN,
                                         10, 10, SC_UI_DEMO_MENU_GAP);
    sc_ui_widget_action_menu_layout_top_right(&demo->action_menu, logical_size);
    sc_ui_widget_action_menu_set_menu_button_enabled(&demo->action_menu, 1,
                                                     false);

    demo->menu_label.rect = sc_ui_widget_menu_get_item_rect(&demo->action_menu.menu,
                                                            0);
    demo->menu_item_one.rect = sc_ui_widget_menu_get_item_rect(&demo->action_menu.menu,
                                                               1);
    demo->menu_separator.rect = sc_ui_widget_menu_get_item_rect(&demo->action_menu.menu,
                                                                2);
    demo->menu_item_two.rect = sc_ui_widget_menu_get_item_rect(&demo->action_menu.menu,
                                                               3);
}

static struct sc_ui_input_result
sc_ui_demo_layer_handle_event(struct sc_ui_layer *layer,
                              struct sc_ui_context *ui,
                              const struct sc_ui_event *event) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);

    struct sc_ui_input_result result = {false, false};
    if (!sc_ui_demo_layer_has_rect(geometry, &demo->primary_button,
                                   &demo->action_menu)) {
        sc_ui_widget_button_reset(&demo->primary_button.button, ui, layer);
        sc_ui_widget_button_reset(&demo->secondary_button, ui, layer);
        sc_ui_widget_button_reset(&demo->menu_item_one, ui, layer);
        sc_ui_widget_button_reset(&demo->menu_item_two, ui, layer);
        demo->menu_open = false;
        return result;
    }

    struct sc_ui_widget_action_menu_result menu_result =
        sc_ui_widget_action_menu_handle_event(&demo->action_menu, ui, layer,
                                              event, demo->menu_open);
    result = menu_result.input;
    if (!result.consumed && demo->menu_open && menu_result.clicked_outside
            && !sc_ui_geom_point_in_rect(event->data.pointer.x,
                                         event->data.pointer.y,
                                         &demo->primary_button.button.rect)) {
        demo->menu_open = false;
        result.request_refresh = true;
    }
    if (result.consumed) {
        return result;
    }

    result = sc_ui_widget_action_button_handle_event(&demo->primary_button, ui,
                                                     layer, event);
    return result;
}

static bool
sc_ui_demo_layer_render(struct sc_ui_layer *layer,
                        const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = render_ctx->geometry;
    if (!sc_ui_demo_layer_has_rect(geometry, &demo->primary_button,
                                   &demo->action_menu)) {
        return true;
    }

    bool ok = sc_ui_widget_action_button_render(&demo->primary_button,
                                                render_ctx);
    ok &= sc_ui_widget_action_menu_render(&demo->action_menu, render_ctx,
                                          demo->menu_open);
    if (demo->menu_open) {
        ok &= sc_ui_widget_label_render(&demo->menu_label, render_ctx);
        ok &= sc_ui_widget_separator_render(&demo->menu_separator, render_ctx);
    }
    return ok;
}

static void
sc_ui_demo_layer_on_detach(struct sc_ui_layer *layer,
                           struct sc_ui_context *ui) {
    (void) ui;
    struct sc_ui_demo_layer *demo = layer->userdata;
    demo->primary_button.button.state.hovered = false;
    demo->primary_button.button.state.pressed = false;
    demo->secondary_button.state.hovered = false;
    demo->secondary_button.state.pressed = false;
    demo->menu_item_one.state.hovered = false;
    demo->menu_item_one.state.pressed = false;
    demo->menu_item_two.state.hovered = false;
    demo->menu_item_two.state.pressed = false;
    demo->menu_open = false;
}

void
sc_ui_demo_layer_init(struct sc_ui_demo_layer *demo) {
    static const struct sc_ui_layer_ops ops = {
        .sync = sc_ui_demo_layer_sync,
        .handle_event = sc_ui_demo_layer_handle_event,
        .render = sc_ui_demo_layer_render,
        .on_detach = sc_ui_demo_layer_on_detach,
    };

    demo->layer = (struct sc_ui_layer) {
        .ops = &ops,
        .visible = true,
        .enabled = true,
        .z_index = 0,
        .userdata = demo,
    };

    sc_ui_widget_action_button_init(&demo->primary_button,
                                    sc_ui_id_from_u32(demo, 1), "DEMO",
                                    sc_ui_demo_layer_action_primary, demo);

    sc_ui_widget_button_init_default(&demo->secondary_button,
                                     sc_ui_id_from_u32(demo, 2), "PING");
    sc_ui_widget_button_init_default(&demo->menu_item_one,
                                     sc_ui_id_from_u32(demo, 3), "FIRST");
    sc_ui_widget_button_init_default(&demo->menu_item_two,
                                     sc_ui_id_from_u32(demo, 4), "SECOND");

    struct sc_ui_widget_button *toolbar_buttons[] = {
        &demo->secondary_button,
    };
    static const sc_ui_widget_action_handler toolbar_actions[] = {
        sc_ui_demo_layer_action_secondary,
    };
    struct sc_ui_widget_button *menu_buttons[] = {
        &demo->menu_item_one,
        &demo->menu_item_two,
    };
    static const sc_ui_widget_action_handler menu_actions[] = {
        sc_ui_demo_layer_action_first,
        sc_ui_demo_layer_action_second,
    };
    sc_ui_widget_action_menu_init(&demo->action_menu, toolbar_buttons,
                                  ARRAY_LEN(toolbar_buttons), toolbar_actions,
                                  menu_buttons, ARRAY_LEN(menu_buttons),
                                  menu_actions, demo);
    sc_ui_widget_action_menu_set_menu_layout_count(&demo->action_menu, 4);
    sc_ui_widget_action_menu_set_menu_button_slot(&demo->action_menu, 0, 1);
    sc_ui_widget_action_menu_set_menu_button_slot(&demo->action_menu, 1, 3);

    struct sc_ui_widget_label_style label_style = {
        .text_style = {
            .color = sc_ui_color_rgba(0xCC, 0xCC, 0xCC, 0xB0),
            .scale = 2,
            .tracking = 2,
        },
    };
    SDL_Rect label_rect = {0, 0, 0, 0};
    sc_ui_widget_label_init(&demo->menu_label, &label_rect, "ACTIONS",
                            &label_style);

    struct sc_ui_widget_separator_style separator_style = {
        .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x40),
        .thickness = 1,
        .inset = 6,
    };
    SDL_Rect separator_rect = {0, 0, 0, 0};
    sc_ui_widget_separator_init(&demo->menu_separator, &separator_rect,
                                &separator_style);

    demo->primary_toggled = false;
    demo->secondary_toggled = false;
    demo->menu_open = false;
}
