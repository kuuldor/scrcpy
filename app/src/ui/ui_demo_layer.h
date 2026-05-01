#ifndef SC_UI_DEMO_LAYER_H
#define SC_UI_DEMO_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui_layer.h"
#include "ui_widget_button.h"
#include "ui_widget_action_menu.h"
#include "ui_widget_label.h"
#include "ui_widget_separator.h"

struct sc_ui_demo_layer {
    struct sc_ui_layer layer;
    struct sc_ui_widget_button primary_button;
    struct sc_ui_widget_action_menu action_menu;
    struct sc_ui_widget_button secondary_button;
    struct sc_ui_widget_label menu_label;
    struct sc_ui_widget_button menu_item_one;
    struct sc_ui_widget_separator menu_separator;
    struct sc_ui_widget_button menu_item_two;
    bool primary_toggled;
    bool secondary_toggled;
    bool menu_open;
};

void
sc_ui_demo_layer_init(struct sc_ui_demo_layer *demo);

#endif
