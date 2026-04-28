#include "ui_demo_layer.h"

#include <SDL2/SDL.h>

#include "ui_context.h"
#include "ui_id.h"
#include "ui_widget_button.h"
#include "ui_widget_toolbar.h"
#include "util/log.h"

#define SC_UI_DEMO_MARGIN 12
#define SC_UI_DEMO_WIDTH 72
#define SC_UI_DEMO_HEIGHT 32
#define SC_UI_DEMO_GAP 10
#define SC_UI_DEMO_TOOLBAR_PADDING 10

static SDL_Rect
sc_ui_demo_layer_get_toolbar_rect(const struct sc_ui_geometry *geometry) {
    return (SDL_Rect) {
        .x = geometry->content_rect.x + SC_UI_DEMO_MARGIN,
        .y = geometry->content_rect.y + SC_UI_DEMO_MARGIN,
        .w = 2 * SC_UI_DEMO_WIDTH + SC_UI_DEMO_GAP
           + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
        .h = SC_UI_DEMO_HEIGHT + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
    };
}

static bool
sc_ui_demo_layer_has_rect(const struct sc_ui_geometry *geometry) {
    SDL_Rect panel = sc_ui_demo_layer_get_toolbar_rect(geometry);
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
        return result;
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
            LOGI("UI demo secondary button clicked (%s)",
                 demo->secondary_toggled ? "toggled on" : "toggled off");
        }
    }

    return result;
}

static bool
sc_ui_demo_layer_render(struct sc_ui_layer *layer,
                        const struct sc_ui_render_ctx *render_ctx) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = render_ctx->geometry;
    if (!sc_ui_demo_layer_has_rect(geometry)) {
        return true;
    }

    demo->toolbar.panel.rect = sc_ui_demo_layer_get_toolbar_rect(geometry);
    demo->primary_button.rect = sc_ui_widget_toolbar_get_rect(&demo->toolbar, 0);
    demo->secondary_button.rect = sc_ui_widget_toolbar_get_rect(&demo->toolbar,
                                                                1);

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

    ok &= sc_ui_widget_button_render(&demo->primary_button, render_ctx);
    ok &= sc_ui_widget_button_render(&demo->secondary_button, render_ctx);
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

    SDL_Rect rect = {
        .x = 0,
        .y = 0,
        .w = SC_UI_DEMO_WIDTH,
        .h = SC_UI_DEMO_HEIGHT,
    };
    SDL_Rect panel_rect = {
        .x = 0,
        .y = 0,
        .w = 2 * SC_UI_DEMO_WIDTH + SC_UI_DEMO_GAP
           + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
        .h = SC_UI_DEMO_HEIGHT + 2 * SC_UI_DEMO_TOOLBAR_PADDING,
    };
    struct sc_ui_widget_toolbar_style toolbar_style = {
        .panel = {
            .fill_color = sc_ui_color_rgba(0x12, 0x12, 0x18, 0xA0),
            .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
            .padding = SC_UI_DEMO_TOOLBAR_PADDING,
        },
        .item_width = SC_UI_DEMO_WIDTH,
        .item_height = SC_UI_DEMO_HEIGHT,
        .item_gap = SC_UI_DEMO_GAP,
    };
    sc_ui_widget_toolbar_init(&demo->toolbar, &panel_rect, &toolbar_style);

    struct sc_ui_widget_button_style style = {
        .fill_color = sc_ui_color_rgba(0x24, 0x56, 0x78, 0xB8),
        .fill_hover_color = sc_ui_color_rgba(0x24, 0x56, 0xB8, 0xB8),
        .fill_pressed_color = sc_ui_color_rgba(0x24, 0x56, 0xD8, 0xB8),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xD0),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 2,
            .tracking = 2,
        },
    };
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

    demo->primary_toggled = false;
    demo->secondary_toggled = false;
}
