#ifndef SC_UI_LAYER_H
#define SC_UI_LAYER_H

#include "common.h"

#include <stdbool.h>

#include "ui_event.h"
#include "ui_render.h"
#include "ui_types.h"

struct sc_ui_context;
struct sc_ui_layer;

struct sc_ui_layer_ops {
    void
    (*sync)(struct sc_ui_layer *layer, struct sc_ui_context *ui);

    struct sc_ui_input_result
    (*handle_event)(struct sc_ui_layer *layer, struct sc_ui_context *ui,
                    const struct sc_ui_event *event);

    bool
    (*render)(struct sc_ui_layer *layer,
              const struct sc_ui_render_ctx *render_ctx);

    void
    (*on_detach)(struct sc_ui_layer *layer, struct sc_ui_context *ui);
};

struct sc_ui_layer {
    const struct sc_ui_layer_ops *ops;

    bool visible;
    bool enabled;
    int z_index;

    void *userdata;
};

#endif
