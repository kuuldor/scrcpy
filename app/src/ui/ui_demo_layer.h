#ifndef SC_UI_DEMO_LAYER_H
#define SC_UI_DEMO_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui_button.h"
#include "ui_layer.h"

struct sc_ui_demo_layer {
    struct sc_ui_layer layer;
    struct sc_ui_button_state button;
    bool toggled;
};

void
sc_ui_demo_layer_init(struct sc_ui_demo_layer *demo);

#endif
