#include "ui_demo_layer.h"

#include <SDL2/SDL.h>

#include "ui_context.h"
#include "ui_id.h"
#include "ui_widget_button.h"
#include "ui_widget_label.h"
#include "ui_widget_menu.h"
#include "ui_widget_separator.h"
#include "ui_widget_toolbar.h"
#include "util/log.h"

#define SC_UI_DEMO_MARGIN 12
#define SC_UI_DEMO_MIN_WIDTH 72
#define SC_UI_DEMO_MIN_HEIGHT 32
#define SC_UI_DEMO_GAP 10
#define SC_UI_DEMO_TOOLBAR_PADDING 10
#define SC_UI_DEMO_MENU_GAP 6
#define SC_UI_DEMO_MENU_PADDING 8

static SDL_Rect
sc_ui_demo_layer_get_menu_rect(const struct sc_ui_demo_layer *demo) {
    SDL_Rect anchor = demo->secondary_button.rect;
    int item_count = 4;
    return (SDL_Rect) {
        .x = anchor.x,
        .y = anchor.y + anchor.h + SC_UI_DEMO_MENU_GAP,
        .w = demo->menu.style.item_width + 2 * demo->menu.style.panel.padding,
        .h = sc_ui_widget_menu_total_height(&demo->menu, item_count),
    };
}

static SDL_Rect
sc_ui_demo_layer_get_toolbar_rect(const struct sc_ui_demo_layer *demo,
                                  const struct sc_ui_geometry *geometry) {
    return (SDL_Rect) {
        .x = geometry->content_rect.x + SC_UI_DEMO_MARGIN,
        .y = geometry->content_rect.y + SC_UI_DEMO_MARGIN,
        .w = sc_ui_widget_toolbar_total_width(&demo->toolbar, 2),
        .h = demo->toolbar.panel.rect.h,
    };
}

static bool
sc_ui_demo_layer_has_rect(const struct sc_ui_demo_layer *demo,
                          const struct sc_ui_geometry *geometry) {
    SDL_Rect panel = sc_ui_demo_layer_get_toolbar_rect(demo, geometry);
    return geometry->has_frame
        && geometry->content_rect.w >= panel.w + 2 * SC_UI_DEMO_MARGIN
        && geometry->content_rect.h >= panel.h + 2 * SC_UI_DEMO_MARGIN;
}

static struct sc_ui_input_result
sc_ui_demo_layer_handle_event(struct sc_ui_layer *layer,
                              struct sc_ui_context *ui,
                              const struct sc_ui_event *event) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);

    struct sc_ui_input_result result = {false, false};
    if (!geometry->has_frame) {
        sc_ui_widget_button_reset(&demo->primary_button, ui, layer);
        sc_ui_widget_button_reset(&demo->secondary_button, ui, layer);
        sc_ui_widget_button_reset(&demo->menu_item_one, ui, layer);
        sc_ui_widget_button_reset(&demo->menu_item_two, ui, layer);
        demo->menu_open = false;
        return result;
    }

    if (demo->menu_open) {
        struct sc_ui_button_result menu_item_one_result =
            sc_ui_widget_button_handle_event(&demo->menu_item_one, ui, layer,
                                             event);
        result = menu_item_one_result.input;
        if (menu_item_one_result.action == SC_UI_BUTTON_ACTION_CLICK) {
            LOGI("UI demo menu item clicked (FIRST)");
            demo->menu_open = false;
            result.request_refresh = true;
        }

        if (!result.consumed) {
            struct sc_ui_button_result menu_item_two_result =
                sc_ui_widget_button_handle_event(&demo->menu_item_two, ui,
                                                 layer, event);
            result = menu_item_two_result.input;
            if (menu_item_two_result.action == SC_UI_BUTTON_ACTION_CLICK) {
                LOGI("UI demo menu item clicked (SECOND)");
                demo->menu_open = false;
                result.request_refresh = true;
            }
        }

        if (!result.consumed && event->type == SC_UI_EVENT_POINTER_DOWN) {
            bool in_menu = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                    event->data.pointer.y,
                                                    &demo->menu.panel.rect);
            bool in_secondary = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                         event->data.pointer.y,
                                                         &demo->secondary_button.rect);
            if (!in_menu && !in_secondary) {
                demo->menu_open = false;
                result.request_refresh = true;
            }
        }

        if (result.consumed) {
            return result;
        }
    }

    struct sc_ui_button_result primary_result =
        sc_ui_widget_button_handle_event(&demo->primary_button, ui, layer,
                                         event);
    result = primary_result.input;
    if (primary_result.action == SC_UI_BUTTON_ACTION_CLICK) {
        demo->primary_toggled = !demo->primary_toggled;
        LOGI("UI demo primary button clicked (%s)",
             demo->primary_toggled ? "toggled on" : "toggled off");
    }

    if (!result.consumed) {
        struct sc_ui_button_result secondary_result =
            sc_ui_widget_button_handle_event(&demo->secondary_button, ui,
                                             layer, event);
        result = secondary_result.input;
        if (secondary_result.action == SC_UI_BUTTON_ACTION_CLICK) {
            demo->secondary_toggled = !demo->secondary_toggled;
            demo->menu_open = !demo->menu_open;
            LOGI("UI demo secondary button clicked (%s, menu %s)",
                 demo->secondary_toggled ? "toggled on" : "toggled off",
                 demo->menu_open ? "opened" : "closed");
            result.request_refresh = true;
        }
    }

    return result;
}

static bool
sc_ui_demo_layer_render(struct sc_ui_layer *layer,
                        const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = render_ctx->geometry;
    if (!sc_ui_demo_layer_has_rect(demo, geometry)) {
        return true;
    }

    demo->toolbar.panel.rect = sc_ui_demo_layer_get_toolbar_rect(demo,
                                                                 geometry);
    demo->primary_button.rect = sc_ui_widget_toolbar_get_rect(&demo->toolbar, 0);
    demo->secondary_button.rect = sc_ui_widget_toolbar_get_rect(&demo->toolbar,
                                                                1);
    demo->menu.panel.rect = sc_ui_demo_layer_get_menu_rect(demo);
    demo->menu_label.rect = sc_ui_widget_menu_get_item_rect(&demo->menu, 0);
    demo->menu_item_one.rect = sc_ui_widget_menu_get_item_rect(&demo->menu, 1);
    demo->menu_separator.rect = sc_ui_widget_menu_get_item_rect(&demo->menu, 2);
    demo->menu_item_two.rect = sc_ui_widget_menu_get_item_rect(&demo->menu, 3);

    bool ok = sc_ui_widget_toolbar_render(&demo->toolbar, render_ctx);

    demo->primary_button.style.fill_color = demo->primary_toggled
                                          ? sc_ui_color_rgba(0x24, 0x8E, 0x78,
                                                             0xB8)
                                          : sc_ui_color_rgba(0x24, 0x56, 0x78,
                                                             0xB8);
    demo->primary_button.style.fill_hover_color = demo->primary_toggled
                                                ? sc_ui_color_rgba(0x24, 0x8E,
                                                                   0xB8, 0xB8)
                                                : sc_ui_color_rgba(0x24, 0x56,
                                                                   0xB8, 0xB8);
    demo->primary_button.style.fill_pressed_color = demo->primary_toggled
                                                  ? sc_ui_color_rgba(0x24, 0x8E,
                                                                     0xD8, 0xB8)
                                                  : sc_ui_color_rgba(0x24, 0x56,
                                                                     0xD8, 0xB8);

    demo->secondary_button.style.fill_color = demo->secondary_toggled
                                            ? sc_ui_color_rgba(0x7A, 0x58, 0x24,
                                                               0xB8)
                                            : sc_ui_color_rgba(0x6A, 0x44, 0x24,
                                                               0xB8);
    demo->secondary_button.style.fill_hover_color = demo->secondary_toggled
                                                  ? sc_ui_color_rgba(0xA0, 0x70,
                                                                     0x24, 0xB8)
                                                  : sc_ui_color_rgba(0x90, 0x58,
                                                                     0x24, 0xB8);
    demo->secondary_button.style.fill_pressed_color = demo->secondary_toggled
                                                    ? sc_ui_color_rgba(0xC0, 0x84,
                                                                       0x24, 0xB8)
                                                    : sc_ui_color_rgba(0xB0, 0x68,
                                                                       0x24, 0xB8);

    struct sc_ui_widget_button_style menu_style = {
        .fill_color = sc_ui_color_rgba(0x22, 0x22, 0x2A, 0xD0),
        .fill_hover_color = sc_ui_color_rgba(0x34, 0x34, 0x46, 0xD8),
        .fill_pressed_color = sc_ui_color_rgba(0x44, 0x44, 0x58, 0xE0),
        .fill_disabled_color = sc_ui_color_rgba(0x2A, 0x2A, 0x30, 0xC0),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x80),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xD0),
        .border_disabled_color = sc_ui_color_rgba(0xB0, 0xB0, 0xB0, 0x70),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 2,
            .tracking = 2,
        },
        .text_disabled_color = sc_ui_color_rgba(0xC8, 0xC8, 0xC8, 0xA8),
        .padding_h = 8,
        .padding_v = 4,
    };
    demo->menu_item_one.style = menu_style;
    demo->menu_item_two.style = menu_style;

    ok &= sc_ui_widget_button_render(&demo->primary_button, render_ctx);
    ok &= sc_ui_widget_button_render(&demo->secondary_button, render_ctx);
    if (demo->menu_open) {
        ok &= sc_ui_widget_menu_render(&demo->menu, render_ctx);
        ok &= sc_ui_widget_label_render(&demo->menu_label, render_ctx);
        ok &= sc_ui_widget_button_render(&demo->menu_item_one, render_ctx);
        ok &= sc_ui_widget_separator_render(&demo->menu_separator, render_ctx);
        ok &= sc_ui_widget_button_render(&demo->menu_item_two, render_ctx);
    }
    return ok;
}

static void
sc_ui_demo_layer_on_detach(struct sc_ui_layer *layer,
                           struct sc_ui_context *ui) {
    (void) ui;
    struct sc_ui_demo_layer *demo = layer->userdata;
    demo->primary_button.state.hovered = false;
    demo->primary_button.state.pressed = false;
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
        .sync = NULL,
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

    struct sc_ui_widget_button_style style = {
        .fill_color = sc_ui_color_rgba(0x24, 0x56, 0x78, 0xB8),
        .fill_hover_color = sc_ui_color_rgba(0x24, 0x56, 0xB8, 0xB8),
        .fill_pressed_color = sc_ui_color_rgba(0x24, 0x56, 0xD8, 0xB8),
        .fill_disabled_color = sc_ui_color_rgba(0x2A, 0x2A, 0x34, 0x98),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xD0),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .border_disabled_color = sc_ui_color_rgba(0xAA, 0xAA, 0xAA, 0x70),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 2,
            .tracking = 2,
        },
        .text_disabled_color = sc_ui_color_rgba(0xC0, 0xC0, 0xC0, 0x90),
        .padding_h = 8,
        .padding_v = 4,
    };
    int32_t toolbar_item_width =
        sc_ui_widget_button_width_for_label("DEMO", &style);
    int32_t ping_width = sc_ui_widget_button_width_for_label("PING", &style);
    if (ping_width > toolbar_item_width) {
        toolbar_item_width = ping_width;
    }
    if (toolbar_item_width < SC_UI_DEMO_MIN_WIDTH) {
        toolbar_item_width = SC_UI_DEMO_MIN_WIDTH;
    }

    int32_t toolbar_item_height = sc_ui_widget_button_height_for_style(&style);
    if (toolbar_item_height < SC_UI_DEMO_MIN_HEIGHT) {
        toolbar_item_height = SC_UI_DEMO_MIN_HEIGHT;
    }

    SDL_Rect rect = {
        .x = 0,
        .y = 0,
        .w = toolbar_item_width,
        .h = toolbar_item_height,
    };

    SDL_Rect panel_rect = {
        .x = 0,
        .y = 0,
        .w = 2 * toolbar_item_width + SC_UI_DEMO_GAP
           + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
        .h = toolbar_item_height + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
    };
    struct sc_ui_widget_toolbar_style toolbar_style = {
        .panel = {
            .fill_color = sc_ui_color_rgba(0x12, 0x12, 0x18, 0xA0),
            .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
            .padding = SC_UI_DEMO_TOOLBAR_PADDING,
        },
        .item_width = toolbar_item_width,
        .item_height = toolbar_item_height,
        .item_gap = SC_UI_DEMO_GAP,
    };
    sc_ui_widget_toolbar_init(&demo->toolbar, &panel_rect, &toolbar_style);

    sc_ui_widget_button_init(&demo->primary_button, sc_ui_id_from_u32(demo, 1),
                             &rect, "DEMO", &style);

    struct sc_ui_widget_button_style secondary_style = style;
    secondary_style.fill_color = sc_ui_color_rgba(0x6A, 0x44, 0x24, 0xB8);
    secondary_style.fill_hover_color = sc_ui_color_rgba(0x90, 0x58, 0x24,
                                                        0xB8);
    secondary_style.fill_pressed_color = sc_ui_color_rgba(0xB0, 0x68, 0x24,
                                                          0xB8);
    sc_ui_widget_button_init(&demo->secondary_button,
                             sc_ui_id_from_u32(demo, 2), &rect, "PING",
                             &secondary_style);

    struct sc_ui_widget_button_style menu_button_style = {
        .fill_color = sc_ui_color_rgba(0x22, 0x22, 0x2A, 0xD0),
        .fill_hover_color = sc_ui_color_rgba(0x34, 0x34, 0x46, 0xD8),
        .fill_pressed_color = sc_ui_color_rgba(0x44, 0x44, 0x58, 0xE0),
        .fill_disabled_color = sc_ui_color_rgba(0x22, 0x22, 0x2A, 0x90),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x80),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xD0),
        .border_disabled_color = sc_ui_color_rgba(0x99, 0x99, 0x99, 0x60),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 2,
            .tracking = 2,
        },
        .text_disabled_color = sc_ui_color_rgba(0xC0, 0xC0, 0xC0, 0x80),
        .padding_h = 8,
        .padding_v = 4,
    };
    int32_t menu_item_width =
        sc_ui_widget_button_width_for_label("SECOND", &menu_button_style);
    if (menu_item_width < toolbar_item_width) {
        menu_item_width = toolbar_item_width;
    }
    int32_t menu_item_height =
        sc_ui_widget_button_height_for_style(&menu_button_style);
    if (menu_item_height < toolbar_item_height) {
        menu_item_height = toolbar_item_height;
    }
    SDL_Rect menu_rect = {
        .x = 0,
        .y = 0,
        .w = menu_item_width + 2 * SC_UI_DEMO_MENU_PADDING,
        .h = 4 * menu_item_height + 3 * SC_UI_DEMO_GAP
           + 2 * SC_UI_DEMO_MENU_PADDING,
    };
    struct sc_ui_widget_menu_style menu_style = {
        .panel = {
            .fill_color = sc_ui_color_rgba(0x10, 0x10, 0x16, 0xD8),
            .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
            .padding = SC_UI_DEMO_MENU_PADDING,
        },
        .item_width = menu_item_width,
        .item_height = menu_item_height,
        .item_gap = SC_UI_DEMO_GAP,
    };
    sc_ui_widget_menu_init(&demo->menu, &menu_rect, &menu_style);

    struct sc_ui_widget_label_style label_style = {
        .text_style = {
            .color = sc_ui_color_rgba(0xCC, 0xCC, 0xCC, 0xB0),
            .scale = 2,
            .tracking = 2,
        },
    };
    int32_t label_width =
        sc_ui_widget_label_width_for_text("ACTIONS", &label_style);
    SDL_Rect label_rect = {
        .x = 0,
        .y = 0,
        .w = label_width,
        .h = sc_ui_widget_label_height_for_style(&label_style),
    };
    sc_ui_widget_label_init(&demo->menu_label, &label_rect, "ACTIONS",
                            &label_style);

    sc_ui_widget_button_init(&demo->menu_item_one,
                             sc_ui_id_from_u32(demo, 3), &rect, "FIRST",
                             &menu_button_style);
    struct sc_ui_widget_separator_style separator_style = {
        .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x40),
        .thickness = 1,
        .inset = 6,
    };
    sc_ui_widget_separator_init(&demo->menu_separator, &rect,
                                &separator_style);
    sc_ui_widget_button_init(&demo->menu_item_two,
                             sc_ui_id_from_u32(demo, 4), &rect, "SECOND",
                             &menu_button_style);
    demo->menu_item_two.enabled = false;

    demo->primary_toggled = false;
    demo->secondary_toggled = false;
    demo->menu_open = false;
}
