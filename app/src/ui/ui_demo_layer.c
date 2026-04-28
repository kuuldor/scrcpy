#include "ui_demo_layer.h"

#include <SDL2/SDL.h>

#include "ui_button.h"
#include "ui_context.h"
#include "ui_draw.h"
#include "ui_id.h"
#include "ui_text.h"

#define SC_UI_DEMO_MARGIN 12
#define SC_UI_DEMO_WIDTH 72
#define SC_UI_DEMO_HEIGHT 32

static SDL_Rect
sc_ui_demo_layer_get_rect(const struct sc_ui_geometry *geometry) {
    return (SDL_Rect) {
        .x = geometry->content_rect.x + SC_UI_DEMO_MARGIN,
        .y = geometry->content_rect.y + SC_UI_DEMO_MARGIN,
        .w = SC_UI_DEMO_WIDTH,
        .h = SC_UI_DEMO_HEIGHT,
    };
}

static bool
sc_ui_demo_layer_has_rect(const struct sc_ui_geometry *geometry) {
    return geometry->has_frame
        && geometry->content_rect.w >= SC_UI_DEMO_WIDTH + 2 * SC_UI_DEMO_MARGIN
        && geometry->content_rect.h >= SC_UI_DEMO_HEIGHT + 2 * SC_UI_DEMO_MARGIN;
}

static struct sc_ui_input_result
sc_ui_demo_layer_handle_event(struct sc_ui_layer *layer,
                              struct sc_ui_context *ui,
                              const struct sc_ui_event *event) {
    struct sc_ui_demo_layer *demo = layer->userdata;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);

    struct sc_ui_input_result result = {false, false};
    if (!geometry->has_frame) {
        sc_ui_button_reset(&demo->button, ui, layer);
        return result;
    }

    SDL_Rect rect = sc_ui_demo_layer_get_rect(geometry);
    struct sc_ui_button_result button_result =
        sc_ui_button_handle_event(&demo->button, ui, layer, &rect, event);
    result = button_result.input;

    if (button_result.action == SC_UI_BUTTON_ACTION_CLICK) {
        demo->toggled = !demo->toggled;
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

    SDL_Rect rect = sc_ui_demo_layer_get_rect(geometry);
    SDL_Renderer *renderer = render_ctx->renderer;

    Uint8 r = 0x24;
    Uint8 g = demo->toggled ? 0x8E : 0x56;
    Uint8 b = demo->button.pressed ? 0xD8
            : demo->button.hovered ? 0xB8
                                   : 0x78;
    Uint8 a = 0xB8;

    bool ok = sc_ui_draw_fill_rect(render_ctx, &rect,
                                   sc_ui_color_rgba(r, g, b, a));
    ok &= sc_ui_draw_rect_border(render_ctx, &rect,
                                 sc_ui_color_rgba(0xFF, 0xFF, 0xFF,
                                                  demo->button.pressed
                                                      ? 0xFF
                                                      : 0xD0));
    struct sc_ui_text_style text_style = {
        .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
        .scale = 2,
        .tracking = 2,
    };
    ok &= sc_ui_text_draw_in_rect(render_ctx, &rect, "DEMO", &text_style,
                                  SC_UI_TEXT_ALIGN_CENTER);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    return ok;
}

static void
sc_ui_demo_layer_on_detach(struct sc_ui_layer *layer,
                           struct sc_ui_context *ui) {
    (void) ui;
    struct sc_ui_demo_layer *demo = layer->userdata;
    demo->button.hovered = false;
    demo->button.pressed = false;
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
    sc_ui_button_init(&demo->button, sc_ui_id_from_u32(demo, 1));
    demo->toggled = false;
}
