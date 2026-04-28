#ifndef SC_UI_TOUCHMAP_LAYER_H
#define SC_UI_TOUCHMAP_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui_layer.h"
#include "ui_widget_button.h"
#include "ui_widget_menu.h"
#include "touchmap_overlay.h"
#include "touchmap_state.h"

struct sc_ui_touchmap_layer {
    struct sc_ui_layer layer;
    struct sc_touchmap_state *touchmap_state;
    struct sc_ui_widget_button edit_button;
    struct sc_ui_widget_panel toolbar_panel;
    struct sc_ui_widget_button toolbar_buttons[3];
    struct sc_ui_widget_menu add_menu;
    struct sc_ui_widget_button add_menu_items[3];
};

void
sc_ui_touchmap_layer_init(struct sc_ui_touchmap_layer *layer,
                           struct sc_touchmap_state *touchmap_state);

#endif