#include "ui_context.h"

#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "util/log.h"

static int
sc_ui_compare_registered_layers(const void *a, const void *b) {
    const struct sc_ui_registered_layer *la = a;
    const struct sc_ui_registered_layer *lb = b;

    if (la->z_index < lb->z_index) {
        return -1;
    }
    if (la->z_index > lb->z_index) {
        return 1;
    }
    return 0;
}

static void
sc_ui_sort_layers(struct sc_ui_context *ui) {
    if (ui->layer_count > 1) {
        qsort(ui->layers, ui->layer_count, sizeof(*ui->layers),
              sc_ui_compare_registered_layers);
    }
}

bool
sc_ui_context_init(struct sc_ui_context *ui,
                   const struct sc_ui_context_params *params) {
    ui->window = params->window;
    ui->enabled = true;
    ui->needs_refresh = false;
    ui->geometry = (struct sc_ui_geometry) {
        .frame_size = {0, 0},
        .content_rect = {0, 0, 0, 0},
        .orientation = SC_ORIENTATION_0,
        .has_frame = false,
    };
    ui->metrics = sc_ui_metrics_make(&ui->geometry);
    ui->hover_id = SC_UI_ID_INVALID;
    ui->active_id = SC_UI_ID_INVALID;
    ui->focus_id = SC_UI_ID_INVALID;
    ui->capture_layer = NULL;
    ui->layers = NULL;
    ui->layer_count = 0;
    return true;
}

void
sc_ui_context_destroy(struct sc_ui_context *ui) {
    sc_ui_context_clear_layers(ui);
    ui->window = NULL;
}

void
sc_ui_context_set_geometry(struct sc_ui_context *ui,
                           const struct sc_ui_geometry *geometry) {
    ui->geometry = *geometry;
    ui->metrics = sc_ui_metrics_make(geometry);
}

const struct sc_ui_geometry *
sc_ui_context_get_geometry(const struct sc_ui_context *ui) {
    return &ui->geometry;
}

const struct sc_ui_metrics *
sc_ui_context_get_metrics(const struct sc_ui_context *ui) {
    return &ui->metrics;
}

void
sc_ui_context_set_enabled(struct sc_ui_context *ui, bool enabled) {
    if (ui->enabled == enabled) {
        return;
    }

    ui->enabled = enabled;
    if (!enabled) {
        sc_ui_context_cancel_interaction(ui);
    }
}

bool
sc_ui_context_is_enabled(const struct sc_ui_context *ui) {
    return ui->enabled;
}

bool
sc_ui_context_add_layer(struct sc_ui_context *ui, struct sc_ui_layer *layer,
                        int z_index) {
    assert(layer);

    struct sc_ui_registered_layer *layers = SDL_realloc(
        ui->layers, (ui->layer_count + 1) * sizeof(*layers));
    if (!layers) {
        LOG_OOM();
        return false;
    }

    ui->layers = layers;
    ui->layers[ui->layer_count] = (struct sc_ui_registered_layer) {
        .layer = layer,
        .z_index = z_index,
    };
    ++ui->layer_count;

    layer->z_index = z_index;
    sc_ui_sort_layers(ui);
    ui->needs_refresh = true;
    return true;
}

void
sc_ui_context_remove_layer(struct sc_ui_context *ui,
                           struct sc_ui_layer *layer) {
    for (size_t i = 0; i < ui->layer_count; ++i) {
        if (ui->layers[i].layer != layer) {
            continue;
        }

        if (layer && layer->ops && layer->ops->on_detach) {
            layer->ops->on_detach(layer, ui);
        }

        if (ui->capture_layer == layer) {
            ui->capture_layer = NULL;
        }

        for (size_t j = i + 1; j < ui->layer_count; ++j) {
            ui->layers[j - 1] = ui->layers[j];
        }
        --ui->layer_count;

        if (!ui->layer_count) {
            SDL_free(ui->layers);
            ui->layers = NULL;
        }

        ui->needs_refresh = true;
        return;
    }
}

void
sc_ui_context_clear_layers(struct sc_ui_context *ui) {
    while (ui->layer_count) {
        sc_ui_context_remove_layer(ui, ui->layers[ui->layer_count - 1].layer);
    }
}

struct sc_ui_input_result
sc_ui_context_handle_event(struct sc_ui_context *ui,
                           const struct sc_ui_event *event) {
    struct sc_ui_input_result result = {false, false};
    if (!ui->enabled) {
        return result;
    }

    struct sc_ui_event transformed_event = *event;
    if (event->type == SC_UI_EVENT_POINTER_MOVE
            || event->type == SC_UI_EVENT_POINTER_DOWN
            || event->type == SC_UI_EVENT_POINTER_UP) {
        struct sc_point logical;
        if (sc_ui_context_drawable_to_logical_point(ui,
                (struct sc_point) {
                    .x = event->data.pointer.x,
                    .y = event->data.pointer.y,
                }, &logical)) {
            transformed_event.data.pointer.x = logical.x;
            transformed_event.data.pointer.y = logical.y;
        }
    }

    for (size_t i = 0; i < ui->layer_count; ++i) {
        struct sc_ui_layer *layer = ui->layers[i].layer;
        if (!layer || !layer->visible || !layer->enabled || !layer->ops) {
            continue;
        }

        if (layer->ops->sync) {
            layer->ops->sync(layer, ui);
        }
    }

    for (size_t i = ui->layer_count; i > 0; --i) {
        struct sc_ui_layer *layer = ui->layers[i - 1].layer;
        if (!layer || !layer->visible || !layer->enabled
                || !layer->ops || !layer->ops->handle_event) {
            continue;
        }

        struct sc_ui_input_result layer_result =
            layer->ops->handle_event(layer, ui, &transformed_event);
        result.request_refresh |= layer_result.request_refresh;
        if (layer_result.consumed) {
            result.consumed = true;
            break;
        }
    }

    if (result.request_refresh) {
        ui->needs_refresh = true;
    }

    return result;
}

bool
sc_ui_context_render(struct sc_ui_context *ui, SDL_Renderer *renderer) {
    if (!ui->enabled) {
        return true;
    }

    struct sc_ui_render_ctx render_ctx = {
        .renderer = renderer,
        .ui = ui,
        .geometry = &ui->geometry,
        .metrics = &ui->metrics,
    };

    for (size_t i = 0; i < ui->layer_count; ++i) {
        struct sc_ui_layer *layer = ui->layers[i].layer;
        if (!layer || !layer->visible || !layer->enabled || !layer->ops) {
            continue;
        }

        if (layer->ops->sync) {
            layer->ops->sync(layer, ui);
        }

        if (layer->ops->render && !layer->ops->render(layer, &render_ctx)) {
            return false;
        }
    }

    ui->needs_refresh = false;
    return true;
}

void
sc_ui_context_request_refresh(struct sc_ui_context *ui) {
    ui->needs_refresh = true;
}

bool
sc_ui_context_needs_refresh(const struct sc_ui_context *ui) {
    return ui->needs_refresh;
}

void
sc_ui_context_clear_refresh(struct sc_ui_context *ui) {
    ui->needs_refresh = false;
}

void
sc_ui_context_cancel_interaction(struct sc_ui_context *ui) {
    ui->hover_id = SC_UI_ID_INVALID;
    ui->active_id = SC_UI_ID_INVALID;
    ui->capture_layer = NULL;
}

void
sc_ui_context_release_pointer_capture(struct sc_ui_context *ui) {
    ui->capture_layer = NULL;
}

void
sc_ui_context_capture_pointer(struct sc_ui_context *ui,
                              struct sc_ui_layer *layer) {
    ui->capture_layer = layer;
}

void
sc_ui_context_release_focus(struct sc_ui_context *ui) {
    ui->focus_id = SC_UI_ID_INVALID;
}

sc_ui_id
sc_ui_context_get_hover_id(const struct sc_ui_context *ui) {
    return ui->hover_id;
}

sc_ui_id
sc_ui_context_get_active_id(const struct sc_ui_context *ui) {
    return ui->active_id;
}

sc_ui_id
sc_ui_context_get_focus_id(const struct sc_ui_context *ui) {
    return ui->focus_id;
}

void
sc_ui_context_set_hover_id(struct sc_ui_context *ui, sc_ui_id id) {
    ui->hover_id = id;
}

void
sc_ui_context_set_active_id(struct sc_ui_context *ui, sc_ui_id id) {
    ui->active_id = id;
}

struct sc_ui_layer *
sc_ui_context_get_capture_layer(const struct sc_ui_context *ui) {
    return ui->capture_layer;
}

bool
sc_ui_context_drawable_to_frame_point(const struct sc_ui_context *ui,
                                      struct sc_point drawable,
                                      struct sc_point *out) {
    const struct sc_ui_geometry *g = &ui->geometry;
    if (!g->has_frame || !g->content_rect.w || !g->content_rect.h
            || !g->frame_size.width || !g->frame_size.height) {
        return false;
    }

    int32_t x = (int64_t) (drawable.x - g->content_rect.x)
              * g->frame_size.width / g->content_rect.w;
    int32_t y = (int64_t) (drawable.y - g->content_rect.y)
              * g->frame_size.height / g->content_rect.h;

    switch (g->orientation) {
        case SC_ORIENTATION_0:
            out->x = x;
            out->y = y;
            break;
        case SC_ORIENTATION_90:
            out->x = y;
            out->y = g->frame_size.width - x;
            break;
        case SC_ORIENTATION_180:
            out->x = g->frame_size.width - x;
            out->y = g->frame_size.height - y;
            break;
        case SC_ORIENTATION_270:
            out->x = g->frame_size.height - y;
            out->y = x;
            break;
        case SC_ORIENTATION_FLIP_0:
            out->x = g->frame_size.width - x;
            out->y = y;
            break;
        case SC_ORIENTATION_FLIP_90:
            out->x = g->frame_size.height - y;
            out->y = g->frame_size.width - x;
            break;
        case SC_ORIENTATION_FLIP_180:
            out->x = x;
            out->y = g->frame_size.height - y;
            break;
        default:
            assert(g->orientation == SC_ORIENTATION_FLIP_270);
            out->x = y;
            out->y = x;
            break;
    }

    return true;
}

bool
sc_ui_context_drawable_to_logical_point(const struct sc_ui_context *ui,
                                        struct sc_point drawable,
                                        struct sc_point *out) {
    const struct sc_ui_geometry *g = &ui->geometry;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(g);
    if (!g->has_frame || !g->content_rect.w || !g->content_rect.h
            || !logical_size.width || !logical_size.height) {
        return false;
    }

    out->x = (int64_t) (drawable.x - g->content_rect.x)
           * logical_size.width / g->content_rect.w;
    out->y = (int64_t) (drawable.y - g->content_rect.y)
           * logical_size.height / g->content_rect.h;
    return true;
}

bool
sc_ui_context_logical_to_drawable_point(const struct sc_ui_context *ui,
                                        struct sc_point logical,
                                        struct sc_point *out) {
    const struct sc_ui_geometry *g = &ui->geometry;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(g);
    if (!g->has_frame || !g->content_rect.w || !g->content_rect.h
            || !logical_size.width || !logical_size.height) {
        return false;
    }

    out->x = g->content_rect.x + (int64_t) logical.x * g->content_rect.w
                              / logical_size.width;
    out->y = g->content_rect.y + (int64_t) logical.y * g->content_rect.h
                              / logical_size.height;
    return true;
}

bool
sc_ui_context_logical_to_drawable_rect(const struct sc_ui_context *ui,
                                       const SDL_Rect *logical,
                                       SDL_Rect *out) {
    struct sc_point origin;
    struct sc_point corner;
    if (!sc_ui_context_logical_to_drawable_point(ui,
            (struct sc_point) {.x = logical->x, .y = logical->y}, &origin)
            || !sc_ui_context_logical_to_drawable_point(ui,
            (struct sc_point) {
                .x = logical->x + logical->w,
                .y = logical->y + logical->h,
            }, &corner)) {
        return false;
    }

    out->x = origin.x;
    out->y = origin.y;
    out->w = corner.x - origin.x;
    out->h = corner.y - origin.y;
    if (out->w <= 0) {
        out->w = 1;
    }
    if (out->h <= 0) {
        out->h = 1;
    }
    return true;
}

int32_t
sc_ui_context_logical_to_drawable_length(const struct sc_ui_context *ui,
                                         int32_t value) {
    const struct sc_ui_geometry *g = &ui->geometry;
    struct sc_size logical_size = sc_ui_geom_get_logical_size(g);
    if (!g->has_frame || !logical_size.width || !logical_size.height
            || !g->content_rect.w || !g->content_rect.h) {
        return value > 0 ? value : 1;
    }

    int32_t sx = (int64_t) value * g->content_rect.w / logical_size.width;
    int32_t sy = (int64_t) value * g->content_rect.h / logical_size.height;
    int32_t scaled = sx < sy ? sx : sy;
    return scaled > 0 ? scaled : 1;
}

bool
sc_ui_context_frame_to_drawable_point(const struct sc_ui_context *ui,
                                      struct sc_point frame,
                                      struct sc_point *out) {
    const struct sc_ui_geometry *g = &ui->geometry;
    if (!g->has_frame || !g->content_rect.w || !g->content_rect.h
            || !g->frame_size.width || !g->frame_size.height) {
        return false;
    }

    int32_t oriented_x;
    int32_t oriented_y;
    switch (g->orientation) {
        case SC_ORIENTATION_0:
            oriented_x = frame.x;
            oriented_y = frame.y;
            break;
        case SC_ORIENTATION_90:
            oriented_x = g->frame_size.width - frame.y;
            oriented_y = frame.x;
            break;
        case SC_ORIENTATION_180:
            oriented_x = g->frame_size.width - frame.x;
            oriented_y = g->frame_size.height - frame.y;
            break;
        case SC_ORIENTATION_270:
            oriented_x = frame.y;
            oriented_y = g->frame_size.height - frame.x;
            break;
        case SC_ORIENTATION_FLIP_0:
            oriented_x = g->frame_size.width - frame.x;
            oriented_y = frame.y;
            break;
        case SC_ORIENTATION_FLIP_90:
            oriented_x = g->frame_size.width - frame.y;
            oriented_y = g->frame_size.height - frame.x;
            break;
        case SC_ORIENTATION_FLIP_180:
            oriented_x = frame.x;
            oriented_y = g->frame_size.height - frame.y;
            break;
        default:
            assert(g->orientation == SC_ORIENTATION_FLIP_270);
            oriented_x = frame.y;
            oriented_y = frame.x;
            break;
    }

    out->x = g->content_rect.x + (int64_t) oriented_x * g->content_rect.w
                              / g->frame_size.width;
    out->y = g->content_rect.y + (int64_t) oriented_y * g->content_rect.h
                              / g->frame_size.height;
    return true;
}

int32_t
sc_ui_context_frame_to_drawable_radius(const struct sc_ui_context *ui,
                                       int32_t radius) {
    const struct sc_ui_geometry *g = &ui->geometry;
    if (!g->has_frame || !g->frame_size.width || !g->content_rect.w) {
        return 0;
    }

    return (int64_t) radius * g->content_rect.w / g->frame_size.width;
}
