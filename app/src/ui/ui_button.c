#include "ui_button.h"

#include <SDL2/SDL_mouse.h>

#include "ui_geom.h"

void
sc_ui_button_init(struct sc_ui_button_state *button, sc_ui_id id) {
    button->id = id;
    button->hovered = false;
    button->pressed = false;
}

void
sc_ui_button_reset(struct sc_ui_button_state *button,
                   struct sc_ui_context *ui,
                   struct sc_ui_layer *layer) {
    button->hovered = false;
    button->pressed = false;
    if (sc_ui_context_get_hover_id(ui) == button->id) {
        sc_ui_context_set_hover_id(ui, SC_UI_ID_INVALID);
    }
    if (sc_ui_context_get_active_id(ui) == button->id) {
        sc_ui_context_set_active_id(ui, SC_UI_ID_INVALID);
    }
    if (sc_ui_context_get_capture_layer(ui) == layer) {
        sc_ui_context_release_pointer_capture(ui);
    }
}

struct sc_ui_button_result
sc_ui_button_handle_event(struct sc_ui_button_state *button,
                          struct sc_ui_context *ui,
                          struct sc_ui_layer *layer,
                          const SDL_Rect *rect,
                          const struct sc_ui_event *event) {
    struct sc_ui_button_result result = {
        .input = {false, false},
        .action = SC_UI_BUTTON_ACTION_NONE,
    };

    switch (event->type) {
        case SC_UI_EVENT_POINTER_MOVE: {
            bool hovered = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                    event->data.pointer.y,
                                                    rect);
            if (hovered != button->hovered) {
                button->hovered = hovered;
                result.input.request_refresh = true;
            }

            if (hovered) {
                sc_ui_context_set_hover_id(ui, button->id);
            } else if (sc_ui_context_get_hover_id(ui) == button->id) {
                sc_ui_context_set_hover_id(ui, SC_UI_ID_INVALID);
            }

            result.input.consumed = hovered
                || (button->pressed && sc_ui_context_get_capture_layer(ui) == layer);
            break;
        }
        case SC_UI_EVENT_POINTER_DOWN: {
            bool inside = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                   event->data.pointer.y,
                                                   rect);
            if (inside && event->data.pointer.button == SDL_BUTTON_LEFT) {
                button->hovered = true;
                button->pressed = true;
                sc_ui_context_set_hover_id(ui, button->id);
                sc_ui_context_set_active_id(ui, button->id);
                sc_ui_context_capture_pointer(ui, layer);
                result.input.consumed = true;
                result.input.request_refresh = true;
                result.action = SC_UI_BUTTON_ACTION_PRESS;
            }
            break;
        }
        case SC_UI_EVENT_POINTER_UP: {
            bool inside = sc_ui_geom_point_in_rect(event->data.pointer.x,
                                                   event->data.pointer.y,
                                                   rect);
            if (button->pressed && event->data.pointer.button == SDL_BUTTON_LEFT) {
                button->pressed = false;
                button->hovered = inside;
                if (inside) {
                    sc_ui_context_set_hover_id(ui, button->id);
                } else if (sc_ui_context_get_hover_id(ui) == button->id) {
                    sc_ui_context_set_hover_id(ui, SC_UI_ID_INVALID);
                }
                if (sc_ui_context_get_active_id(ui) == button->id) {
                    sc_ui_context_set_active_id(ui, SC_UI_ID_INVALID);
                }
                if (sc_ui_context_get_capture_layer(ui) == layer) {
                    sc_ui_context_release_pointer_capture(ui);
                }
                result.input.consumed = true;
                result.input.request_refresh = true;
                result.action = inside ? SC_UI_BUTTON_ACTION_CLICK
                                       : SC_UI_BUTTON_ACTION_RELEASE;
            }
            break;
        }
        case SC_UI_EVENT_CANCEL:
        case SC_UI_EVENT_FOCUS_LOST:
            if (button->hovered || button->pressed) {
                sc_ui_button_reset(button, ui, layer);
                result.input.request_refresh = true;
            }
            break;
        default:
            break;
    }

    return result;
}
