#ifndef SC_UI_TOUCHMAP_LAYER_H
#define SC_UI_TOUCHMAP_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui/ui_layer.h"
#include "ui/ui_widget_action_button.h"
#include "ui/ui_widget_action_menu.h"
#include "touchmap/touchmap_overlay.h"
#include "touchmap/touchmap_state.h"

struct sc_ui_touchmap_layer {
    struct sc_ui_layer layer;
    struct sc_touchmap_state *touchmap_state;
    struct sc_ui_widget_action_button edit_button;
    struct sc_ui_widget_action_menu action_menu;
    struct sc_ui_widget_button toolbar_buttons[3];
    struct sc_ui_widget_button add_menu_items[3];
};

void
sc_ui_touchmap_layer_init(struct sc_ui_touchmap_layer *layer,
                           struct sc_touchmap_state *touchmap_state);

#endif
