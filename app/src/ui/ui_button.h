#ifndef SC_UI_BUTTON_H
#define SC_UI_BUTTON_H

#include "common.h"

#include <stdbool.h>

#include <SDL2/SDL_rect.h>

#include "ui_context.h"

enum sc_ui_button_action {
    SC_UI_BUTTON_ACTION_NONE,
    SC_UI_BUTTON_ACTION_PRESS,
    SC_UI_BUTTON_ACTION_RELEASE,
    SC_UI_BUTTON_ACTION_CLICK,
};

struct sc_ui_button_state {
    sc_ui_id id;
    bool hovered;
    bool pressed;
};

struct sc_ui_button_result {
    struct sc_ui_input_result input;
    enum sc_ui_button_action action;
};

void
sc_ui_button_init(struct sc_ui_button_state *button, sc_ui_id id);

void
sc_ui_button_reset(struct sc_ui_button_state *button,
                   struct sc_ui_context *ui,
                   struct sc_ui_layer *layer);

struct sc_ui_button_result
sc_ui_button_handle_event(struct sc_ui_button_state *button,
                          struct sc_ui_context *ui,
                          struct sc_ui_layer *layer,
                          const SDL_Rect *rect,
                          const struct sc_ui_event *event);

#endif
