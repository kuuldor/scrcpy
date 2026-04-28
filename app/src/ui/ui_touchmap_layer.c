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

static bool
sc_ui_touchmap_layer_is_visible(const struct sc_ui_touchmap_layer *layer) {
    return layer->touchmap_state
        && (layer->touchmap_state->overlay_enabled
            || layer->touchmap_state->edit_mode);
}

static void
sc_ui_touchmap_layer_get_edit_button_text(const struct sc_touchmap_state *state,
                                          char *buf, size_t bufsize) {
    if (!state->map) {
        snprintf(buf, bufsize, "NEW");
    } else if (state->edit_mode) {
        snprintf(buf, bufsize, "QUIT");
    } else {
        snprintf(buf, bufsize, "EDIT");
    }
}

static SDL_Rect
sc_ui_touchmap_layer_get_edit_button_rect(
    const struct sc_ui_touchmap_layer *layer,
    const struct sc_ui_geometry *geometry) {
    return (SDL_Rect) {
        .x = geometry->content_rect.x + geometry->content_rect.w
           - layer->edit_button.rect.w - SC_UI_TOUCHMAP_EDIT_MARGIN,
        .y = geometry->content_rect.y + SC_UI_TOUCHMAP_EDIT_MARGIN,
        .w = layer->edit_button.rect.w,
        .h = layer->edit_button.rect.h,
    };
}

static SDL_Rect
sc_ui_touchmap_layer_get_toolbar_rect(
    const struct sc_ui_touchmap_layer *layer,
    const struct sc_ui_geometry *geometry) {
    return (SDL_Rect) {
        .x = geometry->content_rect.x + geometry->content_rect.w
           - layer->toolbar_panel.rect.w - SC_UI_TOUCHMAP_EDIT_MARGIN,
        .y = geometry->content_rect.y + SC_UI_TOUCHMAP_EDIT_MARGIN,
        .w = layer->toolbar_panel.rect.w,
        .h = layer->toolbar_panel.rect.h,
    };
}

static SDL_Rect
sc_ui_touchmap_layer_get_menu_rect(
    const struct sc_ui_touchmap_layer *layer,
    const struct sc_ui_geometry *geometry) {
    SDL_Rect toolbar_rect = sc_ui_touchmap_layer_get_toolbar_rect(layer,
                                                                    geometry);
    return (SDL_Rect) {
        .x = toolbar_rect.x,
        .y = toolbar_rect.y + toolbar_rect.h + SC_UI_TOUCHMAP_MENU_GAP,
        .w = layer->add_menu.panel.rect.w,
        .h = layer->add_menu.panel.rect.h,
    };
}

static void
sc_ui_touchmap_layer_update_toolbar_button_positions(
    struct sc_ui_touchmap_layer *tm,
    const struct sc_ui_geometry *geometry) {
    SDL_Rect toolbar_rect = sc_ui_touchmap_layer_get_toolbar_rect(tm,
                                                                    geometry);
    tm->toolbar_panel.rect = toolbar_rect;

    int32_t content_x = toolbar_rect.x + SC_UI_TOUCHMAP_TOOLBAR_PADDING;
    int32_t content_y = toolbar_rect.y + SC_UI_TOUCHMAP_TOOLBAR_PADDING;

    for (int i = 0; i < 3; ++i) {
        tm->toolbar_buttons[i].rect.x = content_x
            + i * (tm->toolbar_buttons[i].rect.w + SC_UI_TOUCHMAP_TOOLBAR_GAP);
        tm->toolbar_buttons[i].rect.y = content_y;
    }
}

static struct sc_ui_input_result
sc_ui_touchmap_layer_handle_event(struct sc_ui_layer *layer,
                                  struct sc_ui_context *ui,
                                  const struct sc_ui_event *event) {
    struct sc_ui_touchmap_layer *tm = layer->userdata;
    const struct sc_touchmap_state *state = tm->touchmap_state;
    const struct sc_ui_geometry *geometry = sc_ui_context_get_geometry(ui);

    struct sc_ui_input_result result = {false, false};

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return result;
    }

    if (state->edit_mode) {
        sc_ui_touchmap_layer_update_toolbar_button_positions(tm, geometry);

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
            SDL_Rect menu_rect =
                sc_ui_touchmap_layer_get_menu_rect(tm, geometry);
            tm->add_menu.panel.rect = menu_rect;

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
        SDL_Rect edit_rect =
            sc_ui_touchmap_layer_get_edit_button_rect(tm, geometry);
        tm->edit_button.rect = edit_rect;

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
    const struct sc_ui_geometry *geometry = render_ctx->geometry;

    if (!sc_ui_touchmap_layer_is_visible(tm)) {
        return true;
    }

    bool ok = true;

    char edit_text[16];
    sc_ui_touchmap_layer_get_edit_button_text(state, edit_text,
                                               sizeof(edit_text));

    struct sc_ui_widget_button_style edit_style = {
        .fill_color = sc_ui_color_rgba(0x26, 0x7A, 0x3C, 0xBB),
        .fill_hover_color = sc_ui_color_rgba(0x36, 0x8A, 0x4C, 0xBB),
        .fill_pressed_color = sc_ui_color_rgba(0x16, 0x6A, 0x2C, 0xBB),
        .fill_disabled_color = sc_ui_color_rgba(0x1A, 0x1A, 0x20, 0x90),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xB0),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .border_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x50),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 3,
            .tracking = 3,
        },
        .text_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x80),
        .padding_h = 12,
        .padding_v = 10,
    };
    tm->edit_button.style = edit_style;
    tm->edit_button.label = edit_text;

    if (state->edit_mode) {
        SDL_Rect toolbar_rect =
            sc_ui_touchmap_layer_get_toolbar_rect(tm, geometry);
        tm->toolbar_panel.rect = toolbar_rect;
        ok &= sc_ui_widget_panel_render(&tm->toolbar_panel, render_ctx);

        tm->toolbar_buttons[2].style.fill_color =
            sc_ui_color_rgba(0x9A, 0x2A, 0x2A, 0xBB);
        tm->toolbar_buttons[2].style.fill_hover_color =
            sc_ui_color_rgba(0xAA, 0x3A, 0x3A, 0xBB);
        tm->toolbar_buttons[2].style.fill_pressed_color =
            sc_ui_color_rgba(0x8A, 0x1A, 0x1A, 0xBB);

        for (int i = 0; i < 3; ++i) {
            ok &= sc_ui_widget_button_render(&tm->toolbar_buttons[i],
                                              render_ctx);
        }

        if (state->add_menu_open) {
            SDL_Rect menu_rect =
                sc_ui_touchmap_layer_get_menu_rect(tm, geometry);
            tm->add_menu.panel.rect = menu_rect;

            ok &= sc_ui_widget_menu_render(&tm->add_menu, render_ctx);

            for (size_t i = 0; i < 3; ++i) {
                tm->add_menu_items[i].rect =
                    sc_ui_widget_menu_get_item_rect(&tm->add_menu, i);
                ok &= sc_ui_widget_button_render(&tm->add_menu_items[i],
                                                  render_ctx);
            }
        }
    } else {
        SDL_Rect edit_rect =
            sc_ui_touchmap_layer_get_edit_button_rect(tm, geometry);
        tm->edit_button.rect = edit_rect;
        ok &= sc_ui_widget_button_render(&tm->edit_button, render_ctx);
    }

    return ok;
}

void
sc_ui_touchmap_layer_init(struct sc_ui_touchmap_layer *layer,
                          struct sc_touchmap_state *touchmap_state) {
    static const struct sc_ui_layer_ops ops = {
        .sync = NULL,
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

    struct sc_ui_widget_button_style button_style = {
        .fill_color = sc_ui_color_rgba(0x40, 0x40, 0x50, 0xBB),
        .fill_hover_color = sc_ui_color_rgba(0x50, 0x50, 0x60, 0xBB),
        .fill_pressed_color = sc_ui_color_rgba(0x30, 0x30, 0x40, 0xBB),
        .fill_disabled_color = sc_ui_color_rgba(0x30, 0x30, 0x38, 0x90),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xB0),
        .border_pressed_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xFF),
        .border_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x50),
        .text_style = {
            .color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0xE0),
            .scale = 3,
            .tracking = 3,
        },
        .text_disabled_color = sc_ui_color_rgba(0x80, 0x80, 0x80, 0x80),
        .padding_h = 12,
        .padding_v = 10,
    };

    int32_t button_width =
        sc_ui_widget_button_width_for_label("QUIT", &button_style);
    int32_t button_height =
        sc_ui_widget_button_height_for_style(&button_style);

    int32_t menu_button_width =
        sc_ui_widget_button_width_for_label("BUTTON", &button_style);
    if (menu_button_width > button_width) {
        button_width = menu_button_width;
    }

    SDL_Rect toolbar_rect = {
        .x = 0,
        .y = 0,
        .w = 3 * button_width + 2 * SC_UI_TOUCHMAP_TOOLBAR_GAP
           + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING,
        .h = button_height + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING,
    };
    struct sc_ui_widget_panel_style toolbar_panel_style = {
        .fill_color = sc_ui_color_rgba(0x10, 0x10, 0x16, 0xD8),
        .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
        .padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING,
    };
    sc_ui_widget_panel_init(&layer->toolbar_panel, &toolbar_rect,
                            &toolbar_panel_style);

    SDL_Rect button_rect = {
        .x = 0,
        .y = 0,
        .w = button_width,
        .h = button_height,
    };
    sc_ui_widget_button_init(&layer->edit_button,
                             sc_ui_id_from_u32(layer, 0), &button_rect,
                             "EDIT", &button_style);

    sc_ui_widget_button_init(&layer->toolbar_buttons[0],
                             sc_ui_id_from_u32(layer, 1), &button_rect,
                             "ADD", &button_style);
    sc_ui_widget_button_init(&layer->toolbar_buttons[1],
                             sc_ui_id_from_u32(layer, 2), &button_rect,
                             "DEL", &button_style);
    sc_ui_widget_button_init(&layer->toolbar_buttons[2],
                             sc_ui_id_from_u32(layer, 3), &button_rect,
                             "QUIT", &button_style);

    SDL_Rect menu_panel_rect = {
        .x = 0,
        .y = 0,
        .w = button_width + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING,
        .h = 3 * button_height + 2 * SC_UI_TOUCHMAP_MENU_GAP
           + 2 * SC_UI_TOUCHMAP_TOOLBAR_PADDING,
    };
    struct sc_ui_widget_menu_style menu_style = {
        .panel = {
            .fill_color = sc_ui_color_rgba(0x10, 0x10, 0x16, 0xD8),
            .border_color = sc_ui_color_rgba(0xFF, 0xFF, 0xFF, 0x70),
            .padding = SC_UI_TOUCHMAP_TOOLBAR_PADDING,
        },
        .item_width = button_width,
        .item_height = button_height,
        .item_gap = SC_UI_TOUCHMAP_MENU_GAP,
    };
    sc_ui_widget_menu_init(&layer->add_menu, &menu_panel_rect, &menu_style);

    sc_ui_widget_button_init(&layer->add_menu_items[0],
                             sc_ui_id_from_u32(layer, 4), &button_rect,
                             "BUTTON", &button_style);
    sc_ui_widget_button_init(&layer->add_menu_items[1],
                             sc_ui_id_from_u32(layer, 5), &button_rect,
                             "SKILL", &button_style);
    sc_ui_widget_button_init(&layer->add_menu_items[2],
                             sc_ui_id_from_u32(layer, 6), &button_rect,
                             "WALK", &button_style);
    layer->add_menu_items[2].enabled = false;
}