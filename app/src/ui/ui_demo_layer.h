#ifndef SC_UI_DEMO_LAYER_H
#define SC_UI_DEMO_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui_layer.h"
#include "ui_widget_button.h"
#include "ui_widget_toolbar.h"

struct sc_ui_demo_layer {
    struct sc_ui_layer layer;
    struct sc_ui_widget_toolbar toolbar;
    struct sc_ui_widget_button primary_button;
    struct sc_ui_widget_button secondary_button;
    bool primary_toggled;
    bool secondary_toggled;
};

void
sc_ui_demo_layer_init(struct sc_ui_demo_layer *demo);

#endif
